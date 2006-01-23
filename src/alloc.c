/*
 * Copyright (C) 2003, 2004, 2005, 2006 Robert Lougher <rob@lougher.demon.co.uk>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <errno.h>
#include <limits.h>

#include "jam.h"
#include "alloc.h"
#include "thread.h"

/* Trace GC heap mark/sweep phases - useful for debugging heap
 * corruption */
#ifdef TRACEGC
#define TRACE_GC(x) printf x
#else
#define TRACE_GC(x)
#endif

/* Trace class, object and array allocation */
#ifdef TRACEALLOC
#define TRACE_ALLOC(x) printf x
#else
#define TRACE_ALLOC(x)
#endif

/* Trace object finalization */
#ifdef TRACEFNLZ
#define TRACE_FNLZ(x) printf x
#else
#define TRACE_FNLZ(x)
#endif

/* Object alignment */
#define OBJECT_GRAIN            8

/* Bits used within the chunk header (see also alloc.h) */
#define ALLOC_BIT               1
#define SPECIAL_BIT             4

/* Macros for getting values from the chunk header */
#define HEADER(ptr)             *((uintptr_t*)ptr)
#define HDR_SIZE(hdr)           (hdr & ~(ALLOC_BIT|FLC_BIT|SPECIAL_BIT))
#define HDR_ALLOCED(hdr)        (hdr & ALLOC_BIT)
#define HDR_SPECIAL_OBJ(hdr)    (hdr & SPECIAL_BIT)

/* 1 word header format
  31                                       210
   -------------------------------------------
  |              block size               |   |
   -------------------------------------------
                                             ^ alloc bit
                                            ^ flc bit
                                           ^ special bit
*/

static int verbosegc;

/* Format of an unallocated chunk */
typedef struct chunk {
    uintptr_t header;
    struct chunk *next;
} Chunk;

/* The free list head, and next allocation pointer */
static Chunk *freelist;
static Chunk **chunkpp = &freelist;

/* Heap limits */
static char *heapbase;
static char *heaplimit;
static char *heapmax;

static unsigned long heapfree;

/* The mark bit array, used for marking objects during
   the mark phase.  Allocated on start-up. */
static unsigned int *markBits;
static int markBitSize;

/* List holding objects which need to be finalized */
static Object **has_finaliser_list = NULL;
static int has_finaliser_count     = 0;
static int has_finaliser_size      = 0;

/* The circular list holding finalized objects waiting for
   their finalizer to be ran by the finalizer thread */
static Object **run_finaliser_list = NULL;
static int run_finaliser_start     = 0;
static int run_finaliser_end       = 0;
static int run_finaliser_size      = 0;

/* The circular list holding references to be enqueued
   by the reference handler thread */
static Object **reference_list = NULL;
static int reference_start     = 0;
static int reference_end       = 0;
static int reference_size      = 0;

/* Internal locks protecting the GC lists and heap */
static VMLock heap_lock;
static VMLock has_fnlzr_lock;
static VMWaitLock run_finaliser_lock;
static VMWaitLock reference_lock;

/* A pointer to the finalizer thread. */
static Thread *finalizer_thread;

/* Pre-allocated OutOfMemoryError */
static Object *oom;

/* Cached primitive type array classes -- used to speed up
   primitive array allocation */
static Class *bool_array_class = NULL, *byte_array_class = NULL;
static Class *char_array_class = NULL, *short_array_class = NULL;
static Class *int_array_class = NULL, *float_array_class = NULL;
static Class *double_array_class = NULL, *long_array_class = NULL;

/* Field offsets and method table indexes used for finalization and
   reference handling. Cached in class.c */
extern int ref_referent_offset;
extern int ref_queue_offset;
extern int finalize_mtbl_idx;
extern int enqueue_mtbl_idx;
extern int ldr_vmdata_offset;

/* The possible ways in which a reference may be marked in
   the mark bit array */
#define HARD_MARK               3
#define FINALIZER_MARK          2
#define PHANTOM_MARK            1

#define LIST_INCREMENT          100

#define LOG_BYTESPERMARK        LOG_OBJECT_GRAIN /* 1 mark entry for every OBJECT_GRAIN bytes of heap */
#define BITSPERMARK             2
#define LOG_BITSPERMARK         1
#define LOG_MARKSIZEBITS        5
#define MARKSIZEBITS            32

/* Macros for manipulating the mark bit array */

#define MARKENTRY(ptr)     ((((char*)ptr)-heapbase)>>(LOG_BYTESPERMARK+LOG_MARKSIZEBITS-LOG_BITSPERMARK))
#define MARKOFFSET(ptr)    ((((((char*)ptr)-heapbase)>>LOG_BYTESPERMARK)& \
                                                ((MARKSIZEBITS>>LOG_BITSPERMARK)-1))<<LOG_BITSPERMARK)
#define MARK(ptr,mark)     markBits[MARKENTRY(ptr)]|=mark<<MARKOFFSET(ptr)
#define SET_MARK(ptr,mark) markBits[MARKENTRY(ptr)]=(markBits[MARKENTRY(ptr)]& \
                                                ~(((1<<BITSPERMARK)-1)<<MARKOFFSET(ptr)))|mark<<MARKOFFSET(ptr)

#define IS_MARKED(ptr)     ((markBits[MARKENTRY(ptr)]>>MARKOFFSET(ptr))&((1<<BITSPERMARK)-1))

#define IS_HARD_MARKED(ptr)      (IS_MARKED(ptr) == HARD_MARK)
#define IS_PHANTOM_MARKED(ptr)   (IS_MARKED(ptr) == PHANTOM_MARK)

#define IS_OBJECT(ptr)  (((char*)ptr) > heapbase) && \
                        (((char*)ptr) < heaplimit) && \
                        !(((uintptr_t)ptr)&(OBJECT_GRAIN-1))

#define MIN_OBJECT_SIZE ((sizeof(Object)+HEADER_SIZE+OBJECT_GRAIN-1)&~(OBJECT_GRAIN-1))

void allocMarkBits() {
    int no_of_bits = (heaplimit-heapbase)>>(LOG_BYTESPERMARK-LOG_BITSPERMARK);
    markBitSize = (no_of_bits+MARKSIZEBITS-1)>>LOG_MARKSIZEBITS;

    markBits = (unsigned int *) sysMalloc(markBitSize*sizeof(*markBits));

    TRACE_GC(("Allocated mark bits - size is %d\n", markBitSize));
}

void clearMarkBits() {
    memset(markBits, 0, markBitSize*sizeof(*markBits));
}

void initialiseAlloc(unsigned long min, unsigned long max, int verbose) {

#ifdef USE_MALLOC
    /* Don't use mmap - malloc max heap size */
    char *mem = (char*)malloc(max);
    min = max;
    if(mem == NULL) {
#else
    char *mem = (char*)mmap(0, max, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if(mem == MAP_FAILED) {
#endif
        perror("Aborting the VM -- couldn't allocate the heap");
        exitVM(1);
    }

    /* Align heapbase so that start of heap + HEADER_SIZE is object aligned */
    heapbase = (char*)(((uintptr_t)mem+HEADER_SIZE+OBJECT_GRAIN-1)&~(OBJECT_GRAIN-1))-HEADER_SIZE;

    /* Ensure size of heap is multiple of OBJECT_GRAIN */
    heaplimit = heapbase+((min-(heapbase-mem))&~(OBJECT_GRAIN-1));

    heapmax = heapbase+((max-(heapbase-mem))&~(OBJECT_GRAIN-1));

    freelist = (Chunk*)heapbase;
    freelist->header = heapfree = heaplimit-heapbase;
    freelist->next = NULL;

    TRACE_GC(("Alloced heap size %p\n", heaplimit-heapbase));
    allocMarkBits();

    initVMLock(heap_lock);
    initVMLock(has_fnlzr_lock);
    initVMWaitLock(run_finaliser_lock);
    initVMWaitLock(reference_lock);

    verbosegc = verbose;
}

#define ADD_TO_OBJECT_LIST(list, ob)                                     \
{                                                                        \
    if(list##_start == list##_end) {                                     \
        list##_end = list##_size;                                        \
        list##_start = list##_size += LIST_INCREMENT;                    \
        list##_list = (Object**)sysRealloc(list##_list,                  \
                                           list##_size*sizeof(Object*)); \
    }                                                                    \
    list##_end = list##_end%list##_size;                                 \
    list##_list[list##_end++] = ob;                                      \
}

#define ITERATE_OBJECT_LIST(list, action)                                \
{                                                                        \
    int i;                                                               \
                                                                         \
    if(list##_end > list##_start)                                        \
        for(i = list##_start; i < list##_end; i++)                       \
            action(list##_list[i]);                                      \
    else {                                                               \
        for(i = list##_start; i < list##_size; i++)                      \
            action(list##_list[i]);                                      \
        for(i = 0; i < list##_end; i++)                                  \
            action(list##_list[i]);                                      \
    }                                                                    \
}

extern void markInternedStrings();
extern void markBootClasses();
extern void markLoaderClasses(Object *loader, int mark, int mark_soft_refs);
extern void markJNIGlobalRefs();
extern void scanThreads();
void markChildren(Object *ob, int mark, int mark_soft_refs);

static void doMark(Thread *self, int mark_soft_refs) {
    char *ptr;
    int i, j;

    clearMarkBits();

    if(oom) MARK(oom, HARD_MARK);
    markBootClasses();
    markInternedStrings();
    markJNIGlobalRefs();
    scanThreads();

    /* Grab the run and has finalizer list locks - some thread may
       be inside a suspension-blocked region changing these lists.
       Grabbing ensures any thread has left - they'll self-suspend */

    lockVMLock(has_fnlzr_lock, self);
    unlockVMLock(has_fnlzr_lock, self);
    lockVMWaitLock(run_finaliser_lock, self);

    /* All roots should now be marked.  Scan the heap and recursively
       mark all marked objects - once the heap has been scanned all
       reachable objects should be marked */

    for(ptr = heapbase; ptr < heaplimit;) {
        uintptr_t hdr = HEADER(ptr);
        uintptr_t size = HDR_SIZE(hdr);

#ifdef DEBUG
        printf("Block @%p size %d alloced %d\n", ptr, size, HDR_ALLOCED(hdr));
#endif

        if(HDR_ALLOCED(hdr)) {
            Object *ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_HARD_MARKED(ob))
                markChildren(ob, HARD_MARK, mark_soft_refs);
        }

        /* Skip to next block */
        ptr += size;
    }

    /* Now all reachable objects are marked.  All other objects are garbage.
       Any object with a finalizer which is unmarked, however, must have it's
       finalizer ran before collecting.  Scan the has_finaliser list and move
       all unmarked objects to the run_finaliser list.  This ensures that
       finalizers are ran only once, even if finalization resurrects the
       object, as objects are only added to the has_finaliser list on
       creation */

    for(i = 0, j = 0; i < has_finaliser_count; i++) {
        Object *ob = has_finaliser_list[i];
  
        if(!IS_HARD_MARKED(ob)) {
            ADD_TO_OBJECT_LIST(run_finaliser, ob);
        } else
            has_finaliser_list[j++] = ob;
    }

    /* After scanning, j holds how many finalizers are left */

    if(j != has_finaliser_count) {
        has_finaliser_count = j;

        /* Extra finalizers to be ran, so signal the finalizer thread
           in case it needs waking up.  It won't run until it's
           resumed */

        notifyAllVMWaitLock(run_finaliser_lock, self);
    }

    /* Mark the objects waiting to be finalized.  We must mark them
       as they, and all objects they ref, cannot be deleted until the
       finalizer is ran.   Note,  this includes objects just added,
       and objects that were already on the list - they were found
       to be garbage on a previous gc but we haven't got round to
       finalizing them yet. */

#define RUN_MARK(element) \
         markChildren(element, FINALIZER_MARK, mark_soft_refs)

    ITERATE_OBJECT_LIST(run_finaliser, RUN_MARK);

    /* Release the run finaliser lock.  Thread must be suspended */
    unlockVMWaitLock(run_finaliser_lock, self);

    /* Grab the reference list lock.  This ensures the reference
       handler isn't in the middle of modifying the list */
    lockVMWaitLock(reference_lock, self);

    /* There may be references still waiting to be enqueued by the
       reference handler (from a previous GC).  Remove them if
       they're now unreachable as they will be collected */

#define CLEAR_UNMARKED(element) \
         if(element && !IS_MARKED(element)) element = NULL

    ITERATE_OBJECT_LIST(reference, CLEAR_UNMARKED);
}

static uintptr_t doSweep(Thread *self) {
    char *ptr;
    Chunk newlist;
    Chunk *curr, *last = &newlist;

    /* Will hold the size of the largest free chunk
       after scanning */
    uintptr_t largest = 0;

    /* Variables used to store verbose gc info */
    uintptr_t marked = 0, unmarked = 0, freed = 0, cleared = 0;

    /* Amount of free heap is re-calculated during scan */
    heapfree = 0;

    /* Scan the heap and free all unmarked objects by reconstructing
       the freelist.  Add all free chunks and unmarked objects and
       merge adjacent free chunks into contiguous areas */

    for(ptr = heapbase; ptr < heaplimit; ) {
        uintptr_t hdr = HEADER(ptr);
        uintptr_t size = HDR_SIZE(hdr);
        Object *ob;

        if(HDR_ALLOCED(hdr)) {
            ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_MARKED(ob))
                goto marked;

            freed += size;
            unmarked++;

            if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL) {
                if(IS_CLASS(ob)) {
                    if(verbosegc) {
                        ClassBlock *cb = CLASS_CB(ob);
                        if(!IS_CLASS_DUP(cb))
                            printf("<GC: Unloading class %s>\n", cb->name);
                    }
                    freeClassData((Class*)ob);
                } else
                    if(IS_CLASS_LOADER(CLASS_CB(ob->class))) {
                        TRACE_GC(("FREE: Freeing class loader object\n"));
                        freeClassLoaderData(ob);
                    }
            }

            TRACE_GC(("FREE: Freeing ob @%p class %s - start of block\n", ob,
                                    ob->class ? CLASS_CB(ob->class)->name : "?"));
        }
        else
            TRACE_GC(("FREE: Unalloced block @%p size %d - start of block\n", ptr, size));
        
        curr = (Chunk *) ptr;

        /* Clear any set bits within the header */
        curr->header &= ~(ALLOC_BIT|FLC_BIT|SPECIAL_BIT);

        /* Scan the next chunks - while they are
           free, merge them onto the first free
           chunk */

        for(;;) {
            ptr += size;

            if(ptr>=heaplimit)
                goto out_last_free;

            hdr = HEADER(ptr);
            size = HDR_SIZE(hdr);
            if(HDR_ALLOCED(hdr)) {
                ob = (Object*)(ptr+HEADER_SIZE);

                if(IS_MARKED(ob))
                    break;

                freed += size;
                unmarked++;

                if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL) {
                    if(IS_CLASS(ob)) {
                        if(verbosegc) {
                            ClassBlock *cb = CLASS_CB(ob);
                            if(!IS_CLASS_DUP(cb))
                                printf("<GC: Unloading class %s>\n", cb->name);
                        }
                        freeClassData((Class*)ob);
                    } else
                        if(IS_CLASS_LOADER(CLASS_CB(ob->class))) {
                            TRACE_GC(("FREE: Freeing class loader object\n"));
                            freeClassLoaderData(ob);
                        }
                }

                TRACE_GC(("FREE: Freeing object @%p class %s - merging onto block @%p\n",
                                       ob, ob->class ? CLASS_CB(ob->class)->name : "?", curr));

            }
            else 
                TRACE_GC(("FREE: unalloced block @%p size %d - merging onto block @%p\n", ptr, size, curr));
            curr->header += size;
        }

        /* Scanned to next marked object see
           if it's the largest so far */
        if(curr->header > largest)
            largest = curr->header;

        /* Add onto total count of free chunks */
        heapfree += curr->header;

    /* Add chunk onto the freelist only if it's
       large enough to hold an object */
        if(curr->header >= MIN_OBJECT_SIZE) {
            last->next = curr;
            last = curr;
        }

marked:
        marked++;

        if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL) {
            ClassBlock *cb = CLASS_CB(ob->class);

            if(IS_REFERENCE(cb)) {
                Object *referent = (Object *)INST_DATA(ob)[ref_referent_offset];

                if(referent != NULL) {
                    int ref_mark = IS_MARKED(referent);

                    TRACE_GC(("FREE: found Reference Object @%p class %s flags %d referent %x mark %d\n",
                              ob, cb->name, cb->flags, referent, ref_mark));

                    if(IS_PHANTOM_REFERENCE(cb)) {
                        if(ref_mark != PHANTOM_MARK)
                            goto out;
                    } else {
                        if(ref_mark == HARD_MARK)
                            goto out;

                        TRACE_GC(("FREE: Clearing the referent field.\n"));
                        INST_DATA(ob)[ref_referent_offset] = 0;
                        cleared++;
                    }

                    /* If the reference has a queue, add it to the list for enqueuing
                       by the Reference Handler thread. */

                    if((Object *)INST_DATA(ob)[ref_queue_offset] != NULL) {
                        TRACE_GC(("FREE: Adding to list for enqueuing.\n"));
                        ADD_TO_OBJECT_LIST(reference, ob);
                    }
                }
            }
        }

out:
        /* Skip to next block */
        ptr += size;

        if(ptr >= heaplimit)
            goto out_last_marked;
    }

out_last_free:

    /* Last chunk is free - need to check if
       largest */
    if(curr->header > largest)
        largest = curr->header;

    heapfree += curr->header;

    /* Add chunk onto the freelist only if it's
       large enough to hold an object */
    if(curr->header >= MIN_OBJECT_SIZE) {
        last->next = curr;
        last = curr;
    }

out_last_marked:

    /* We've now reconstructed the freelist, set freelist
       pointer to new list */
    last->next = NULL;
    freelist = newlist.next;

    /* Reset next allocation block to beginning of list -
       this leads to a search - use largest instead? */
    chunkpp = &freelist;

#ifdef DEBUG
{
    Chunk *c;
    for(c = freelist; c != NULL; c = c->next)
        printf("Chunk @%p size: %d\n", c, c->header);
}
#endif
 
    /* Notify the reference handler.  If there's no
       work to be done it'll go back to sleep */
    notifyAllVMWaitLock(reference_lock, self);
    unlockVMWaitLock(reference_lock, self);

    if(verbosegc) {
        long long size = heaplimit-heapbase;
        long long pcnt_used = ((long long)heapfree)*100/size;
        printf("<GC: Allocated objects: %lld>\n", (long long)marked);
        printf("<GC: Freed %lld object(s) using %lld bytes",
			(long long)unmarked, (long long)freed);
        if(cleared)
            printf(", cleared %lld reference(s)", (long long)cleared);
        printf(">\n<GC: Largest block is %lld total free is %lld out of %lld (%lld%%)>\n",
                         (long long)largest, (long long)heapfree, size, pcnt_used);
    }

    /* Return the size of the largest free chunk in heap - this
       is the largest allocation request that can be satisfied */

    return largest;
}

/* Run all outstanding finalizers.  Finalizers are only ran by the
   finalizer thread, so the current thread waits for the finalizer
   to finish.  Although the JLS allows arbitrary threads to run
   finalizers, this is inherently dangerous as locks maybe held,
   leading to deadlock. */

#define TIMEOUT 100 /* milliseconds */

static void runFinalizers0(Thread *self, int max_wait) {
    int i, size, old_size;

    /* If this is the finalizer thread we've been called
       from within a finalizer -- don't wait for ourselves! */
    if(self == finalizer_thread)
        return;

    lockVMWaitLock(run_finaliser_lock, self);

    /* Wait for the finalizer thread to finish running all
       outstanding finalizers. Rare possibility that a finalizer
       may try to grab a lock we're holding.  To avoid deadlock
       use a timeout and give up if the finalizer's made no
       foward progress. */

    old_size = run_finaliser_size + 1;

    for(i = 0; i < max_wait/TIMEOUT; i++) {
        size = run_finaliser_end - run_finaliser_start;
        if(size <= 0)
            size += run_finaliser_size;

        if(size == 0 || size >= old_size)
            break;

        old_size = size;
        timedWaitVMWaitLock(run_finaliser_lock, self, TIMEOUT);
    }

    unlockVMWaitLock(run_finaliser_lock, self);
}

/* Called by VMRuntime.runFinalization() -- runFinalizers0
   is entered with suspension disabled. */

void runFinalizers() {
    Thread *self = threadSelf();
    disableSuspend(self);
    runFinalizers0(self, 100000);
    enableSuspend(self);
}

static void getTime(struct timeval *tv) {
    gettimeofday(tv, 0);
}

static long endTime(struct timeval *start) {
    struct timeval end;
    int secs, usecs;

    getTime(&end);
    usecs = end.tv_usec - start->tv_usec;
    secs = end.tv_sec - start->tv_sec;

    return secs * 1000000 + usecs;
}

unsigned long gc0(int mark_soft_refs) {
    Thread *self = threadSelf();
    uintptr_t largest;

    disableSuspend(self);
    suspendAllThreads(self);

    if(verbosegc) {
        struct timeval start;
        float scan_time;
        float mark_time;

        getTime(&start);
        doMark(self, mark_soft_refs);
        scan_time = endTime(&start)/1000000.0;

        getTime(&start);
        largest = doSweep(self);
        mark_time = endTime(&start)/1000000.0;

        printf("<GC: Mark took %f seconds, scan took %f seconds>\n", scan_time, mark_time);
    } else {
        doMark(self, mark_soft_refs);
        largest = doSweep(self);
    }

    resumeAllThreads(self);
    enableSuspend(self);

    return largest;
}

void gc1() {
    Thread *self;
    disableSuspend(self = threadSelf());
    lockVMLock(heap_lock, self);
    enableSuspend(self);
    gc0(TRUE);
    unlockVMLock(heap_lock, self);
}

void expandHeap(int min) {
    Chunk *chunk, *new;
    uintptr_t delta;

    if(verbosegc)
        printf("<GC: Expanding heap - minimum needed is %d>\n", min);

    delta = (heaplimit-heapbase)/2;
    delta = delta < min ? min : delta;

    if((heaplimit + delta) > heapmax)
        delta = heapmax - heaplimit;

    /* Ensure new region is multiple of object grain in size */

    delta = (delta&~(OBJECT_GRAIN-1));

    if(verbosegc)
        printf("<GC: Expanding heap by %lld bytes>\n", (long long)delta);

    /* The freelist is in address order - find the last
       free chunk and add the new area to the end.  */

    for(chunk = freelist; chunk->next != NULL; chunk = chunk->next);

    new = (Chunk*)heaplimit;
    new->header = delta;
    new->next = NULL;

    chunk->next = new;
    heaplimit += delta;
    heapfree += delta;

    /* The heap has increased in size - need to reallocate
       the mark bits to cover new area */

    free(markBits);
    allocMarkBits();
}

void *gcMalloc(int len) {
    /* The state determines what action to take in the event of
       allocation failure.  The states go up in seriousness,
       and are visible to other threads */
    static enum { gc, run_finalizers, throw_oom } state = gc;

    int n = (len+HEADER_SIZE+OBJECT_GRAIN-1)&~(OBJECT_GRAIN-1);
    uintptr_t largest;
    Chunk *found;
    Thread *self;
#ifdef TRACEALLOC
    int tries;
#endif

    /* See comment below */
    char *ret_addr;

    /* Grab the heap lock, hopefully without having to
       wait for it to avoid disabling suspension */
    self = threadSelf();
    if(!tryLockVMLock(heap_lock, self)) {
        disableSuspend(self);
        lockVMLock(heap_lock, self);
        enableSuspend(self);
    }

    /* Scan freelist looking for a chunk big enough to
       satisfy allocation request */

    for(;;) {
#ifdef TRACEALLOC
       tries = 0;
#endif
        while(*chunkpp) {
            uintptr_t len = (*chunkpp)->header;

            if(len == n) {
                found = *chunkpp;
                *chunkpp = found->next;
                goto got_it;
            }

            if(len > n) {
                Chunk *rem;
                found = *chunkpp;
                rem = (Chunk*)((char*)found + n);
                rem->header = len - n;

                /* Chain the remainder onto the freelist only
                   if it's large enough to hold an object */
                if(rem->header >= MIN_OBJECT_SIZE) {
                    rem->next = found->next;
                    *chunkpp = rem;
                } else
                    *chunkpp = found->next;

                goto got_it;
            }
            chunkpp = &(*chunkpp)->next;
#ifdef TRACEALLOC
            tries++;
#endif
        }

        if(verbosegc)
            printf("<GC: Alloc attempt for %d bytes failed.>\n", n);

        switch(state) {

            case gc:
                /* Normal failure.  Do a garbage-collection and retry
                   allocation if the largest block satisfies the request.
                   Attempt to ensure heap is at least 25% free, to stop
                   rapid gc cycles */
                largest = gc0(TRUE);
                if(n <= largest && (heapfree * 4 >= (heaplimit - heapbase)))
                    break;

                /* We fall through into the next state, but we need to set
                   the state as it will be visible to other threads */
                state = run_finalizers;

            case run_finalizers:
                /* Before expanding heap try to run outstanding finalizers.
                   If gc found new finalizers, this gives the finalizer chance
                   to run them */
                unlockVMLock(heap_lock, self);
                disableSuspend(self);

                if(verbosegc)
                    printf("<GC: Waiting for finalizers to be ran.>\n");

                runFinalizers0(self, 200);
                lockVMLock(heap_lock, self);
                enableSuspend(self);

                if(state != run_finalizers)
                    break;

                /* Retry gc (as above) */
                largest = gc0(TRUE);
                if(n <= largest && (heapfree * 4 >= (heaplimit - heapbase))) {
                    state = gc;
                    break;
                }

                /* Still not freed enough memory so try to expand the heap.
                   Note we retry allocation even if the heap couldn't be
                   expanded sufficiently -- there's a chance gc may merge
                   adjacent blocks together at the top of the heap */
                if(heaplimit < heapmax) {
                    expandHeap(n);
                    state = gc;
                    break;
                }

                if(verbosegc)
                    printf("<GC: Stack at maximum already.  Clearing Soft References>\n");

                /* Can't expand the heap any more.  Try GC again but this time
                   clearing all soft references.  Note we succeed if we can satisfy
                   the request -- we may have been able to all along, but with
                   nothing spare.  We may thrash, but it's better than throwing OOM */
                largest = gc0(FALSE);
                if(n <= largest) {
                    state = gc;
                    break;
                }

                if(verbosegc)
                    printf("<GC: completely out of heap space - throwing OutOfMemoryError>\n");

                state = throw_oom;
                unlockVMLock(heap_lock, self);
                signalException("java/lang/OutOfMemoryError", NULL);
                return NULL;
                break;

            case throw_oom:
                /* Already throwing an OutOfMemoryError in some thread.  In both
                 * cases, throw an already prepared OOM (no stacktrace).  Could have a
                 * per-thread flag, so we try to throw a new OOM in each thread, but
                 * if we're this low on memory I doubt it'll make much difference.
                 */

                if(verbosegc)
                    printf("<GC: completely out of heap space - throwing prepared OutOfMemoryError>\n");

                state = gc;
                unlockVMLock(heap_lock, self);
                setException(oom);
                return NULL;
                break;
        }
    }

got_it:
#ifdef TRACEALLOC
    printf("<ALLOC: took %d tries to find block.>\n", tries);
#endif

    heapfree -= n;

    /* Mark found chunk as allocated */
    found->header = n | ALLOC_BIT;

    /* Found is a block pointer - if we unlock now, small window
     * where new object ref is not held and will therefore be gc'ed.
     * Setup ret_addr before unlocking to prevent this.
     */
   
    ret_addr = ((char*)found)+HEADER_SIZE;
    memset(ret_addr, 0, n-HEADER_SIZE);
    unlockVMLock(heap_lock, self);

    return ret_addr;
}

/* Object roots :-
        classes
        class statics
        interned strings
        thread Java stacks
        thread C stacks
        thread registers (put onto C stack)
        JNI refs
            - locals (scanned as part of JavaStack)
            - globals
*/

void markClassData(Class *class, int mark, int mark_soft_refs) {
    ClassBlock *cb = CLASS_CB(class);
    FieldBlock *fb = cb->fields;
    int i;

    TRACE_GC(("Marking class %s\n", cb->name));

    if(cb->class_loader != NULL && mark > IS_MARKED(cb->class_loader))
        markChildren(cb->class_loader, mark, mark_soft_refs);

    TRACE_GC(("Marking static fields for class %s\n", cb->name));

    /* Static fields are initialised to default values during
       preparation (done in the link phase).  Therefore, don't
       scan if the class hasn't been linked */
    if(cb->state < CLASS_LINKED)
        return;

    for(i = 0; i < cb->fields_count; i++, fb++)
        if((fb->access_flags & ACC_STATIC) &&
                    ((*fb->type == 'L') || (*fb->type == '['))) {
            Object *ob = (Object *)fb->static_value;
            TRACE_GC(("Field %s %s object @%p\n", fb->name, fb->type, ob));
            if(ob != NULL && mark > IS_MARKED(ob))
                markChildren(ob, mark, mark_soft_refs);
        }
}

void scanThread(Thread *thread) {
    ExecEnv *ee = thread->ee;
    Frame *frame = ee->last_frame;
    uintptr_t *end, *slot;

    TRACE_GC(("Scanning stacks for thread 0x%x id %d\n", thread, thread->id));

    MARK(ee->thread, HARD_MARK);

    /* If there's a pending exception raised
       on this thread mark it */
    if(ee->exception)
        MARK(ee->exception, HARD_MARK);

    slot = (uintptr_t*)getStackTop(thread);
    end = (uintptr_t*)getStackBase(thread);

    for(; slot < end; slot++)
        if(IS_OBJECT(*slot)) {
            Object *ob = (Object*)*slot;
            TRACE_GC(("Found C stack ref @%p object ref is %p\n", slot, ob));
            MARK(ob, HARD_MARK);
        }

    slot = frame->ostack + frame->mb->max_stack;

    while(frame->prev != NULL) {
        if(frame->mb != NULL) {
            TRACE_GC(("Scanning %s.%s\n", CLASS_CB(frame->mb->class)->name, frame->mb->name));
            TRACE_GC(("lvars @%p ostack @%p\n", frame->lvars, frame->ostack));

            /* Mark the method's defining class.  This should always
               be reachable otherwise, but mark to be safe */
            MARK(frame->mb->class, HARD_MARK);
        }

        end = frame->ostack;

        for(; slot >= end; slot--)
            if(IS_OBJECT(*slot)) {
                Object *ob = (Object*)*slot;
                TRACE_GC(("Found Java stack ref @%p object ref is %p\n", slot, ob));
                MARK(ob, HARD_MARK);
            }

        slot -= sizeof(Frame)/sizeof(uintptr_t);
        frame = frame->prev;
    }
}

void markObject(Object *object, int mark, int mark_soft_refs) {
    if(object != NULL && mark > IS_MARKED(object))
        markChildren(object, mark, mark_soft_refs);
}

void markRoot(Object *object) {
    if(object != NULL)
        MARK(object, HARD_MARK);
}

void markChildren(Object *ob, int mark, int mark_soft_refs) {
    Class *class = ob->class;
    ClassBlock *cb = CLASS_CB(class);

    SET_MARK(ob, mark);

    if(class == NULL)
        return;

    if(mark > IS_MARKED(class))
        markChildren((Object*)class, mark, mark_soft_refs);

    if(cb->name[0] == '[') {
        if((cb->name[1] == 'L') || (cb->name[1] == '[')) {
            Object **body = ARRAY_DATA(ob);
            int len = ARRAY_LEN(ob);
            int i;
            TRACE_GC(("Scanning Array object @%p class is %s len is %d\n", ob, cb->name, len));

            for(i = 0; i < len; i++) {
                Object *ob = *body++;
                TRACE_GC(("Object at index %d is @%p\n", i, ob));

                if(ob != NULL && mark > IS_MARKED(ob))
                    markChildren(ob, mark, mark_soft_refs);
            }
        } else {
            TRACE_GC(("Array object @%p class is %s  - Not Scanning...\n", ob, cb->name));
        }
    } else {
        uintptr_t *body = INST_DATA(ob);
        int i;

        if(IS_CLASS(ob)) {
            TRACE_GC(("Found class object @%p name is %s\n", ob, CLASS_CB(ob)->name));
            markClassData((Class*)ob, mark, mark_soft_refs);
        } else
            if(IS_CLASS_LOADER(cb)) {
                TRACE_GC(("Mark found class loader object @%p class %s\n", ob, cb->name));
                markLoaderClasses(ob, mark, mark_soft_refs);
            } else
                if(IS_VMTHROWABLE(cb)) {
                    TRACE_GC(("Mark found VMThrowable object @%p\n", ob));
                    markVMThrowable(ob, mark, mark_soft_refs);
                } else
                    if(IS_REFERENCE(cb)) {
                        Object *referent = (Object *)body[ref_referent_offset];

                        TRACE_GC(("Mark found Reference object @%p class %s flags %d referent %p\n",
                                 ob, cb->name, cb->flags, referent));

                        if(!IS_WEAK_REFERENCE(cb) && referent != NULL) {
                            int ref_mark = IS_MARKED(referent);
                            int new_mark;

                            if(IS_PHANTOM_REFERENCE(cb))
                                new_mark = PHANTOM_MARK;
                            else
                                if(!IS_SOFT_REFERENCE(cb) || mark_soft_refs)
                                    new_mark = mark;
                                else
                                    new_mark = 0;

                            if(new_mark > ref_mark) {
                                TRACE_GC(("Marking referent object @%p mark %d ref_mark %d new_mark %d\n",
                                                                        referent, mark, ref_mark, new_mark));
                                markChildren(referent, new_mark, mark_soft_refs);
                            }
                        }
                    }

        TRACE_GC(("Scanning object @%p class is %s\n", ob, cb->name));

        /* The reference offsets table consists of a list of start and
           end offsets corresponding to the references within the object's
           instance data.  Scan the list, and mark all references. */

        for(i = 0; i < cb->refs_offsets_size; i++) {
            int offset = cb->refs_offsets_table[i].start;
            int end = cb->refs_offsets_table[i].end;

            for(; offset < end; offset++) {
                Object *ob = (Object *)body[offset];
                TRACE_GC(("Offset %d reference @%p\n", offset, ob));

                if(ob != NULL && mark > IS_MARKED(ob))
                    markChildren(ob, mark, mark_soft_refs);
            }
        }
    }
}


/* Routines to retrieve snapshot of heap status */

unsigned long freeHeapMem() {
    return heapfree;
}

unsigned long totalHeapMem() {
    return heaplimit-heapbase;
}

unsigned long maxHeapMem() {
    return heapmax-heapbase;
}

/* The async gc loop.  It sleeps for 1 second and
 * calls gc if the system's idle and the heap's
 * changed */

void asyncGCThreadLoop(Thread *self) {
    for(;;) {
        threadSleep(self, 1000, 0);
        if(systemIdle(self))
            gc1();
    }
}

#define PROCESS_OBJECT_LIST(list, method_idx, verbose_message, self, stack_top) \
{                                                                               \
    disableSuspend0(self, stack_top);                                           \
    lockVMWaitLock(list##_lock, self);                                          \
                                                                                \
    for(;;) {                                                                   \
        waitVMWaitLock(list##_lock, self);                                      \
                                                                                \
        if((list##_start == list##_size) && (list##_end == 0))                  \
            continue;                                                           \
                                                                                \
        if(verbosegc) {                                                         \
            int diff = list##_end - list##_start;                               \
            printf(verbose_message, diff > 0 ? diff : diff + list##_size);      \
        }                                                                       \
                                                                                \
        do {                                                                    \
            Object *ob;                                                         \
            list##_start %= list##_size;                                        \
	    if((ob = list##_list[list##_start]) == NULL)                        \
                continue;                                                       \
                                                                                \
            unlockVMWaitLock(list##_lock, self);                                \
            enableSuspend(self);                                                \
                                                                                \
            /* Run the process method */                                        \
            executeMethod(ob, CLASS_CB(ob->class)->method_table[method_idx]);   \
                                                                                \
            /* Should be nothing interesting on stack or in                     \
             * registers so use same stack top as thread start. */              \
                                                                                \
            disableSuspend0(self, stack_top);                                   \
            lockVMWaitLock(list##_lock, self);                                  \
                                                                                \
            /* Clear any exceptions - exceptions thrown in finalizers are       \
               silently ignored */                                              \
                                                                                \
            clearException();                                                   \
        } while(++list##_start != list##_end);                                  \
                                                                                \
        list##_start = list##_size;                                             \
        list##_end = 0;                                                         \
                                                                                \
        notifyAllVMWaitLock(list##_lock, self);                                 \
    }                                                                           \
}

/* The finalizer thread waits for notification
 * of new finalizers (by the thread doing gc)
 * and then runs them */

void finalizerThreadLoop(Thread *self) {
    finalizer_thread = self;
    PROCESS_OBJECT_LIST(run_finaliser, finalize_mtbl_idx,
                        "<GC: running %d finalisers>\n", self, &self);
}

/* The reference handler thread waits for notification
   by the GC of new reference objects, and enqueues
   them */

void referenceHandlerThreadLoop(Thread *self) {
    PROCESS_OBJECT_LIST(reference, enqueue_mtbl_idx,
                        "<GC: enqueuing %d references>\n", self, &self);
}

void initialiseGC(int noasyncgc) {
    /* Pre-allocate an OutOfMemoryError exception object - we throw it
     * when we're really low on heap space, and can create FA... */

    MethodBlock *init;
    Class *oom_clazz = findSystemClass("java/lang/OutOfMemoryError");
    if(exceptionOccured()) {
        printException();
        exitVM(1);
    }

    init = lookupMethod(oom_clazz, "<init>", "(Ljava/lang/String;)V");
    oom = allocObject(oom_clazz);
    executeMethod(oom, init, NULL);

    /* Create and start VM threads for the reference handler and finalizer */
    createVMThread("Finalizer", finalizerThreadLoop);
    createVMThread("Reference Handler", referenceHandlerThreadLoop);

    /* Create and start VM thread for asynchronous GC */
    if(!noasyncgc)
        createVMThread("Async GC", asyncGCThreadLoop);
}

/* Object allocation routines */

#define ADD_FINALIZED_OBJECT(ob)                                                   \
{                                                                                  \
    Thread *self;                                                                  \
    disableSuspend(self = threadSelf());                                           \
    lockVMLock(has_fnlzr_lock, self);                                              \
    TRACE_FNLZ(("Object @%p type %s has a finalize method...\n",                   \
                                                  ob, CLASS_CB(ob->class)->name)); \
    if(has_finaliser_count == has_finaliser_size) {                                \
        has_finaliser_size += LIST_INCREMENT;                                      \
        has_finaliser_list = (Object**)sysRealloc(has_finaliser_list,              \
                                               has_finaliser_size*sizeof(Object*));\
    }                                                                              \
                                                                                   \
    has_finaliser_list[has_finaliser_count++] = ob;                                \
    unlockVMLock(has_fnlzr_lock, self);                                            \
    enableSuspend(self);                                                           \
}

Object *allocObject(Class *class) {
    ClassBlock *cb = CLASS_CB(class);
    int size = cb->object_size * sizeof(uintptr_t);
    Object *ob = (Object *)gcMalloc(size + sizeof(Object));

    if(ob != NULL) {
        ob->class = class;

        /* If the object needs finalising add it to the
           has finaliser list */

        if(IS_FINALIZED(cb))
            ADD_FINALIZED_OBJECT(ob);

        /* If the object is an instance of a special class
           mark it by setting the bit in the chunk header */

        if(IS_SPECIAL(cb)) {
            uintptr_t *hdr = (uintptr_t*)(((char*)ob)-HEADER_SIZE);
            *hdr |= SPECIAL_BIT;
        }

        TRACE_ALLOC(("<ALLOC: allocated %s object @%p>\n", cb->name, ob));
    }

    return ob;
}
    
Object *allocArray(Class *class, int size, int el_size) {
    Object *ob;

    if(size > (INT_MAX - sizeof(u4) - sizeof(Object)) / el_size) {
        signalException("java/lang/OutOfMemoryError", NULL);
        return NULL;
    }

    ob = (Object *)gcMalloc(size * el_size + sizeof(u4) + sizeof(Object));

    if(ob != NULL) {
        *(u4*)INST_DATA(ob) = size;
        ob->class = class;
        TRACE_ALLOC(("<ALLOC: allocated %s array object @%p>\n", CLASS_CB(class)->name, ob));
    }

    return ob;
}

Object *allocTypeArray(int type, int size) {
    Class *class;
    int el_size;

    if(size < 0) {
        signalException("java/lang/NegativeArraySizeException", NULL);
        return NULL;
    }

    switch(type) {
        case T_BOOLEAN:
            if(bool_array_class == NULL)
                bool_array_class = findArrayClass("[Z");
            class = bool_array_class;
            el_size = 1;
            break;

        case T_BYTE:
            if(byte_array_class == NULL)
                byte_array_class = findArrayClass("[B");
            class = byte_array_class;
            el_size = 1;
            break;

        case T_CHAR:
            if(char_array_class == NULL)
                char_array_class = findArrayClass("[C");
            class = char_array_class;
            el_size = 2;
            break;

        case T_SHORT:
            if(short_array_class == NULL)
                short_array_class = findArrayClass("[S");
            class = short_array_class;
            el_size = 2;
            break;

        case T_INT:
            if(int_array_class == NULL)
                int_array_class = findArrayClass("[I");
            class = int_array_class;
            el_size = 4;
            break;

        case T_FLOAT:
            if(float_array_class == NULL)
                float_array_class = findArrayClass("[F");
            class = float_array_class;
            el_size = 4;
            break;

        case T_DOUBLE:
            if(double_array_class == NULL)
                double_array_class = findArrayClass("[D");
            class = double_array_class;
            el_size = 8;
            break;

        case T_LONG:
            if(long_array_class == NULL)
                long_array_class = findArrayClass("[J");
            class = long_array_class;
            el_size = 8;
            break;

        default:
            printf("Invalid array type %d - aborting VM...\n", type);
            exit(0);
    }

    if(class == NULL)
        return NULL;

    return allocArray(class, size, el_size);
}

Object *allocMultiArray(Class *array_class, int dim, intptr_t *count) {

    int i;
    Object *array;
    char *element_name = CLASS_CB(array_class)->name + 1;

    if(dim > 1) {
        Class *aclass = findArrayClassFromClass(element_name, array_class);
        Object **body;

        array = allocArray(array_class, *count, sizeof(Object*));

        if(array == NULL)
            return NULL;

        body = ARRAY_DATA(array);
        for(i = 0; i < *count; i++)
            if((*body++ = allocMultiArray(aclass, dim - 1, count + 1)) == NULL)
                return NULL;
    } else {
        int el_size;

        switch(*element_name) {
            case 'B':
            case 'Z':
                el_size = 1;
                break;

            case 'C':
            case 'S':
                el_size = 2;
                break;

            case 'I':
            case 'F':
                el_size = 4;
                break;

            case 'L':
                el_size = sizeof(Object*);
                break;

            default:
                el_size = 8;
                break;
        }
        array = allocArray(array_class, *count, el_size);        
    }

    return array;
}

Class *allocClass() {
    Class *class = (Class*)gcMalloc(sizeof(ClassBlock)+sizeof(Class));
    uintptr_t *hdr = (uintptr_t*)(((char*)class)-HEADER_SIZE);
    *hdr |= SPECIAL_BIT;

    TRACE_ALLOC(("<ALLOC: allocated class object @%p>\n", class));
    return class; 
}

Object *cloneObject(Object *ob) {
    unsigned int hdr = HEADER((((char*)ob)-HEADER_SIZE));
    int size = HDR_SIZE(hdr)-HEADER_SIZE;
    Object *clone = (Object*)gcMalloc(size);

    if(clone != NULL) {
        memcpy(clone, ob, size);

        clone->lock = 0;
        MBARRIER();

        if(IS_FINALIZED(CLASS_CB(clone->class)))
            ADD_FINALIZED_OBJECT(clone);

        if(HDR_SPECIAL_OBJ(hdr)) {
            uintptr_t *clone_hdr = (uintptr_t*)(((char*)clone)-HEADER_SIZE);
            *clone_hdr |= SPECIAL_BIT;

            if(IS_CLASS_LOADER(CLASS_CB(clone->class)))
                INST_DATA(clone)[ldr_vmdata_offset] = 0;
        }

        TRACE_ALLOC(("<ALLOC: cloned object @%p clone @%p>\n", ob, clone));
    }

    return clone;
}

void *sysMalloc(int n) {
    void *mem = malloc(n);

    if(mem == NULL) {
        fprintf(stderr, "Malloc failed - aborting VM...\n");
        exitVM(1);
    }

    return mem;
}

void *sysRealloc(void *ptr, int n) {
    void *mem = realloc(ptr, n);

    if(mem == NULL) {
        fprintf(stderr, "Realloc failed - aborting VM...\n");
        exitVM(1);
    }

    return mem;
}

