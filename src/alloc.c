/*
 * Copyright (C) 2003, 2004, 2005 Robert Lougher <rob@lougher.demon.co.uk>.
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
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
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
#include "arch.h"

/* Trace GC heap mark/sweep phases - useful for debugging heap
 * corruption */
#ifdef TRACEGC
#define TRACE_GC(x) printf x; fflush(stdout)
#else
#define TRACE_GC(x)
#endif

/* Trace class, object and array allocation */
#ifdef TRACEALLOC
#define TRACE_ALLOC(x) printf x; fflush(stdout)
#else
#define TRACE_ALLOC(x)
#endif

/* Trace object finalization */
#ifdef TRACEFNLZ
#define TRACE_FNLZ(x) printf x; fflush(stdout)
#else
#define TRACE_FNLZ(x)
#endif

#define OBJECT_GRAIN            8
#define ALLOC_BIT               1

#define HEADER(ptr)             *((unsigned int*)ptr)
#define HDR_SIZE(hdr)           (hdr & ~(ALLOC_BIT|FLC_BIT))
#define HDR_ALLOCED(hdr)        (hdr & ALLOC_BIT)

/* 1 word header format
  31                                       210
   -------------------------------------------
  |              block size               |   |
   -------------------------------------------
                                             ^ alloc bit
                                            ^ flc bit
*/

static int verbosegc;

typedef struct chunk {
    unsigned int header;
    struct chunk *next;
} Chunk;

static Chunk *freelist;
static Chunk **chunkpp = &freelist;

static char *heapbase;
static char *heaplimit;
static char *heapmax;

static int heapfree;

static unsigned int *markBits;
static int markBitSize;

static Object **has_finaliser_list = NULL;
static int has_finaliser_count    = 0;
static int has_finaliser_size     = 0;

static Object **run_finaliser_list = NULL;
static int run_finaliser_start    = 0;
static int run_finaliser_end      = 0;
static int run_finaliser_size     = 0;

static VMLock heap_lock;
static VMLock has_fnlzr_lock;
static VMWaitLock run_fnlzr_lock;

static Object *oom;

#define LIST_INCREMENT          1000

#define LOG_BYTESPERBIT         LOG_OBJECT_GRAIN /* 1 mark bit for every OBJECT_GRAIN bytes of heap */
#define LOG_MARKSIZEBITS        5
#define MARKSIZEBITS            32

#define MARKENTRY(ptr)  ((((char*)ptr)-heapbase)>>(LOG_BYTESPERBIT+LOG_MARKSIZEBITS))
#define MARKOFFSET(ptr) (((((char*)ptr)-heapbase)>>LOG_BYTESPERBIT)&(MARKSIZEBITS-1))
#define MARK(ptr)       markBits[MARKENTRY(ptr)]|=1<<MARKOFFSET(ptr)
#define IS_MARKED(ptr)  (markBits[MARKENTRY(ptr)]&(1<<MARKOFFSET(ptr)))

#define IS_OBJECT(ptr)  (((char*)ptr) > heapbase) && \
                        (((char*)ptr) < heaplimit) && \
                        !(((unsigned int)ptr)&(OBJECT_GRAIN-1))

void allocMarkBits() {
    int no_of_bits = (heaplimit-heapbase)>>LOG_BYTESPERBIT;
    markBitSize = (no_of_bits+MARKSIZEBITS-1)>>LOG_MARKSIZEBITS;

    markBits = (unsigned int *) malloc(markBitSize*sizeof(*markBits));

    TRACE_GC(("Allocated mark bits - size is %d\n", markBitSize));
}

void clearMarkBits() {
    memset(markBits, 0, markBitSize*sizeof(*markBits));
}

void initialiseAlloc(int min, int max, int verbose) {

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
    heapbase = (char*)(((int)mem+HEADER_SIZE+OBJECT_GRAIN-1)&~(OBJECT_GRAIN-1))-HEADER_SIZE;

    /* Ensure size of heap is multiple of OBJECT_GRAIN */
    heaplimit = heapbase+((min-(heapbase-mem))&~(OBJECT_GRAIN-1));

    heapmax = heapbase+((max-(heapbase-mem))&~(OBJECT_GRAIN-1));

    freelist = (Chunk*)heapbase;
    freelist->header = heapfree = heaplimit-heapbase;
    freelist->next = NULL;

    TRACE_GC(("Alloced heap size 0x%x\n",heaplimit-heapbase));
    allocMarkBits();

    initVMLock(heap_lock);
    initVMLock(has_fnlzr_lock);
    initVMWaitLock(run_fnlzr_lock);

    verbosegc = verbose;
}

extern void markInternedStrings();
extern void markClasses();
extern void markJNIGlobalRefs();
extern void scanThreads();
void markChildren(Object *ob);

static void doMark(Thread *self) {
    char *ptr;
    int i, j;

    clearMarkBits();

    if(oom) MARK(oom);
    markClasses();
    markInternedStrings();
    markJNIGlobalRefs();
    scanThreads();

    /* Grab the run and has finalizer list locks - some thread may
       be inside a suspension-blocked region changing these lists.
       Grabbing ensures any thread has left - they'll self-suspend */

    lockVMLock(has_fnlzr_lock, self);
    unlockVMLock(has_fnlzr_lock, self);
    lockVMWaitLock(run_fnlzr_lock, self);

    /* Mark any objects waiting to be finalized - they were found to
       be garbage on a previous gc but we haven't got round to finalizing
       them yet - we must mark them as they, and all objects they ref,
       cannot be deleted until the finalizer is ran... */

    if(run_finaliser_end > run_finaliser_start)
        for(i = run_finaliser_start; i < run_finaliser_end; i++)
            MARK(run_finaliser_list[i]);
    else {
        for(i = run_finaliser_start; i < run_finaliser_size; i++)
            MARK(run_finaliser_list[i]);
        for(i = 0; i < run_finaliser_end; i++)
            MARK(run_finaliser_list[i]);
    }

    /* All roots should now be marked.  Scan the heap and recursively
       mark all marked objects - once the heap has been scanned all
       reachable objects should be marked */

    for(ptr = heapbase; ptr < heaplimit;) {
        unsigned int hdr = HEADER(ptr);
        int size = HDR_SIZE(hdr);

#ifdef DEBUG
        printf("Block @0x%x size %d alloced %d\n", ptr, size, HDR_ALLOCED(hdr));
#endif

        if(HDR_ALLOCED(hdr)) {
            Object *ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_MARKED(ob))
                markChildren(ob);
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
  
        if(!IS_MARKED(ob)) {
            markChildren(ob);
            if(run_finaliser_start == run_finaliser_end) {
                run_finaliser_start = 0;
                run_finaliser_end = run_finaliser_size;
                run_finaliser_size += LIST_INCREMENT;
                run_finaliser_list = (Object**)sysRealloc(run_finaliser_list,
                                run_finaliser_size*sizeof(Object*));
            }
            run_finaliser_end = run_finaliser_end%run_finaliser_size;
            run_finaliser_list[run_finaliser_end++] = ob;
        } else {
            has_finaliser_list[j++] = ob;
        }        
    }

    /* After scanning, j holds how many finalizers are left */

    if(j != has_finaliser_count) {
        has_finaliser_count = j;

        /* Extra finalizers to be ran, so signal the finalizer thread
           in case it needs waking up.  It won't run until it's
           resumed */

        notifyVMWaitLock(run_fnlzr_lock, self);
    }
    unlockVMWaitLock(run_fnlzr_lock, self);
}

static int doSweep(Thread *self) {
    char *ptr;
    Chunk newlist;
    Chunk *last = &newlist;

    /* Will hold the size of the largest free chunk
       after scanning */
    int largest = 0;

    /* Variables used to store verbose gc info */
    int marked = 0, unmarked = 0, freed = 0;

    /* Amount of free heap is re-calculated during scan */
    heapfree = 0;

    /* Scan the heap and free all unmarked objects by reconstructing
       the freelist.  Add all free chunks and unmarked objects and
       merge adjacent free chunks into contiguous areas */

    for(ptr = heapbase; ptr < heaplimit; ) {
        unsigned int hdr = HEADER(ptr);
        int size = HDR_SIZE(hdr);

        if(HDR_ALLOCED(hdr)) {
            Object *ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_MARKED(ob))
                goto marked;

            freed += size;
            unmarked++;

            TRACE_GC(("FREE: Freeing ob @ 0x%x class %s - start of block\n", ob,
                                    ob->class ? CLASS_CB(ob->class)->name : "?"));
        }
        else
            TRACE_GC(("FREE: Unalloced block @ 0x%x size %d - start of block\n", ptr, size));
        
        /* Add chunk onto the freelist */
        last->next = (Chunk *) ptr;
        last = last->next;

        /* Clear the alloc and flc bits in the header */
        last->header &= ~(ALLOC_BIT|FLC_BIT);

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
                Object *ob = (Object*)(ptr+HEADER_SIZE);

                if(IS_MARKED(ob))
                    break;

                freed += size;
                unmarked++;

                TRACE_GC(("FREE: Freeing object @ 0x%x class %s - merging onto block @ 0x%x\n",
                                       ob, ob->class ? CLASS_CB(ob->class)->name : "?", last));

            }
            else 
                TRACE_GC(("FREE: unalloced block @ 0x%x size %d - merging onto block @ 0x%x\n", ptr, size, last));
            last->header += size;
        }

        /* Scanned to next marked object see if it's
           the largest so far */

        if(last->header > largest)
            largest = last->header;

        /* Add onto total count of free chunks */
        heapfree += last->header;

marked:
        marked++;

        /* Skip to next block */
        ptr += size;

        if(ptr >= heaplimit)
            goto out_last_marked;
    }

out_last_free:

    /* Last chunk is free - need to check if
       largest */
    if(last->header > largest)
        largest = last->header;

    heapfree += last->header;

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
        printf("Chunk @0x%x size: %d\n", c, c->header);
}
#endif

    if(verbosegc) {
        int size = heaplimit-heapbase;
        long long pcnt_used = ((long long)heapfree)*100/size;
        printf("<GC: Allocated objects: %d>\n", marked);
        printf("<GC: Freed %d objects using %d bytes>\n", unmarked, freed);
        printf("<GC: Largest block is %d total free is %d out of %d (%lld%%)>\n",
                         largest, heapfree, size, pcnt_used);
    }

    /* Return the size of the largest free chunk in heap - this
       is the largest allocation request that can be satisfied */

    return largest;
}

/* Run all outstanding finalizers.  This is called synchronously
 * if we run out of memory and asyncronously by the finalizer
 * thread.  In both cases suspension is disabled, to reduce
 * back-to-back enable/disable... */

static Thread *runningFinalizers = NULL;
static int runFinalizers() {
    Thread *self = threadSelf();
    int ret = FALSE;

    lockVMWaitLock(run_fnlzr_lock, self);

    if(runningFinalizers == self)
        goto out;

    while(runningFinalizers) {
        ret = TRUE;
        waitVMWaitLock(run_fnlzr_lock, self);
    }

    if((run_finaliser_start == run_finaliser_size) && (run_finaliser_end == 0))
        goto out;

    runningFinalizers = self;

    TRACE_FNLZ(("run_finaliser_start %d\n",run_finaliser_start));
    TRACE_FNLZ(("run_finaliser_end %d\n",run_finaliser_end));
    TRACE_FNLZ(("run_finaliser_size %d\n",run_finaliser_size));

    if(verbosegc) {
        int diff = run_finaliser_end - run_finaliser_start;
        printf("<Running %d finalizers>\n", diff > 0 ? diff : diff + run_finaliser_size);
    }

    do {
        Object *ob;
        run_finaliser_start %= run_finaliser_size;
        ob = run_finaliser_list[run_finaliser_start];

        unlockVMWaitLock(run_fnlzr_lock, self);
        enableSuspend(self);

        /* Run the finalizer method */
        executeMethod(ob, CLASS_CB(ob->class)->finalizer);

        /* We entered with suspension off - nothing
         * else interesting on stack, so use previous
         * stack top. */

        disableSuspend0(self, getStackTop(self));
        lockVMWaitLock(run_fnlzr_lock, self);

        /* Clear any exceptions - exceptions thrown in finalizers are
           silently ignored */

        clearException();
    } while(++run_finaliser_start != run_finaliser_end);

    run_finaliser_start = run_finaliser_size;
    run_finaliser_end = 0;
    runningFinalizers = NULL;

    ret = TRUE;
out:
    notifyVMWaitLock(run_fnlzr_lock, self);
    unlockVMWaitLock(run_fnlzr_lock, self);
    return ret;
}

static long getTime() {
   struct timeval tv;

   gettimeofday(&tv, 0);
   return tv.tv_usec;
}

static long endTime(long start) {
    long time = getTime() - start;

    return time < 0 ? 1000000-time : time;
}

int gc0() {
    Thread *self = threadSelf();
    long start;
    float scan_time;
    float mark_time;
    int largest;

    suspendAllThreads(self);

    start = getTime();
    doMark(self);
    scan_time = endTime(start)/1000000.0;

    start = getTime();
    largest = doSweep(self);
    mark_time = endTime(start)/1000000.0;

    resumeAllThreads(self);

    if(verbosegc)
        printf("<GC: Mark took %f seconds, scan took %f seconds>\n", scan_time, mark_time);

    return largest;
}

void gc1() {
    Thread *self;
    disableSuspend(self = threadSelf());
    lockVMLock(heap_lock, self);
    gc0();
    unlockVMLock(heap_lock, self);
    enableSuspend(self);
}

void expandHeap(int min) {
    Chunk *chunk, *new;
    int delta;

    if(verbosegc)
        printf("<GC: Expanding heap - minimum needed is %d>\n", min);

    delta = (heaplimit-heapbase)/2;
    delta = delta < min ? min : delta;

    if((heaplimit + delta) > heapmax)
        delta = heapmax - heaplimit;

    /* Ensure new region is multiple of object grain in size */

    delta = (delta&~(OBJECT_GRAIN-1));

    if(verbosegc)
        printf("<GC: Expanding heap by %d bytes>\n", delta);

    /* The freelist is in address order - find the last
       free chunk and add the new area to the end.  */

    for(chunk = freelist; chunk->next != NULL; chunk = chunk->next);

    new = (Chunk*)heaplimit;
    new->header = delta;
    new->next = 0;

    chunk->next = new;
    heaplimit += delta;
    heapfree += delta;

    /* The heap has increased in size - need to reallocate
       the mark bits to cover new area */

    free(markBits);
    allocMarkBits();
}

void *gcMalloc(int len) {
    static int state = 0; /* allocation failure action */

    int n = (len+HEADER_SIZE+OBJECT_GRAIN-1)&~(OBJECT_GRAIN-1);
    Chunk *found;
    int largest;
    Thread *self;
#ifdef TRACEALLOC
    int tries;
#endif

    /* See comment below */
    char *ret_addr;

    disableSuspend(self = threadSelf());
    lockVMLock(heap_lock, self);

    /* Scan freelist looking for a chunk big enough to
       satisfy allocation request */

    for(;;) {
#ifdef TRACEALLOC
       tries = 0;
#endif
        while(*chunkpp) {
            int len = (*chunkpp)->header;

            if(len == n) {
                found = *chunkpp;
                *chunkpp = found->next;
                goto gotIt;
            }

            if(len > n) {
                Chunk *rem;
                found = *chunkpp;
                rem = (Chunk*)((char*)found + n);
                rem->header = len - n;
                rem->next = found->next;
                *chunkpp = rem;
                goto gotIt;
            }
            chunkpp = &(*chunkpp)->next;
#ifdef TRACEALLOC
            tries++;
#endif
        }

        if(verbosegc)
            printf("<GC: Alloc attempt for %d bytes failed (state %d).>\n", n, state);

        switch(state) {

            case 0:
                largest = gc0();
                if(n <= largest)
                    break;

                state = 1;

            case 1: {
                int res;

                unlockVMLock(heap_lock, self);
                res = runFinalizers();
                lockVMLock(heap_lock, self);

                if(state == 1) {
                    if(res) {
                        largest = gc0();
                        if(n <= largest) {
                            state = 0;
                            break;
                        }
                    }
                    state = 2;
                }
                break;

            case 2:
                if(heaplimit < heapmax) {
                    expandHeap(n);
                    state = 0;
                    break;
                } else {
                    if(verbosegc)
                        printf("<GC: Stack at maximum already - completely out of heap space>\n");

                    state = 3;
                    /* Reset next allocation block
                     * to beginning of list */
                    chunkpp = &freelist;
                    enableSuspend(self);
                    unlockVMLock(heap_lock, self);
                    signalException("java/lang/OutOfMemoryError", NULL);
                    return NULL;
                }
                break;
            }

            case 3:
                /* Already throwing an OutOfMemoryError in some thread.  In both
                 * cases, throw an already prepared OOM (no stacktrace).  Could have a
                 * per-thread flag, so we try to throw a new OOM in each thread, but
                 * if we're this low on memory I doubt it'll make much difference.
                 */

                state = 0;
                enableSuspend(self);
                unlockVMLock(heap_lock, self);
                setException(oom);
                return NULL;
                break;
        }
    }

gotIt:
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
    enableSuspend(self);
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

void markClassStatics(Class *class) {
    ClassBlock *cb = CLASS_CB(class);
    FieldBlock *fb = cb->fields;
    int i;

    TRACE_GC(("Marking static fields for class %s\n", cb->name));

    for(i = 0; i < cb->fields_count; i++, fb++)
        if((fb->access_flags & ACC_STATIC) &&
                    ((*fb->type == 'L') || (*fb->type == '['))) {
            Object *ob = (Object *)fb->static_value;
            TRACE_GC(("Field %s %s\n", fb->name, fb->type));
            TRACE_GC(("Object @0x%x is valid %d\n", ob, IS_OBJECT(ob)));
            if(IS_OBJECT(ob) && !IS_MARKED(ob))
                markChildren(ob);
        }
}

void scanThread(Thread *thread) {
    ExecEnv *ee = thread->ee;
    Frame *frame = ee->last_frame;
    u4 *end, *slot;

    TRACE_GC(("Scanning stacks for thread 0x%x\n", thread));

    MARK(ee->thread);

    slot = (u4*)getStackTop(thread);
    end = (u4*)getStackBase(thread);

    for(; slot < end; slot++)
        if(IS_OBJECT(*slot)) {
            Object *ob = (Object*)*slot;
            TRACE_GC(("Found C stack ref @0x%x object ref is 0x%x\n", slot, ob));
            MARK(ob);
        }

    slot = frame->ostack + frame->mb->max_stack;

    while(frame->prev != NULL) {
        if(frame->mb != NULL) {
            TRACE_GC(("Scanning %s.%s\n", CLASS_CB(frame->mb->class)->name, frame->mb->name));
            TRACE_GC(("lvars @0x%x ostack @0x%x\n", frame->lvars, frame->ostack));
        }

        end = frame->ostack;

        for(; slot >= end; slot--)
            if(IS_OBJECT(*slot)) {
                Object *ob = (Object*)*slot;
                TRACE_GC(("Found Java stack ref @0x%x object ref is 0x%x\n", slot, ob));
                MARK(ob);
            }

        slot -= sizeof(Frame)/4;
        frame = frame->prev;
    }
}

void markObject(Object *object) {
    if(IS_OBJECT(object))
        MARK(object);
}

void markChildren(Object *ob) {

    MARK(ob);

    if(ob->class == NULL)
        return;
 
    if(IS_CLASS(ob)) {
        TRACE_GC(("Found class object @0x%x name is %s\n", ob, CLASS_CB(ob)->name));
        markClassStatics((Class*)ob);
    } else {
        Class *class = ob->class;
        ClassBlock *cb = CLASS_CB(class);
        u4 *body = INST_DATA(ob);

        if(cb->name[0] == '[') {
            if((cb->name[1] == 'L') || (cb->name[1] == '[')) {
                int len = body[0];
                int i;
                TRACE_GC(("Scanning Array object @0x%x class is %s len is %d\n", ob, cb->name, len));

                for(i = 1; i <= len; i++) {
                    Object *ob = (Object *)body[i];
                    TRACE_GC(("Object at index %d is @0x%x is valid %d\n", i-1, ob, IS_OBJECT(ob)));

                    if(IS_OBJECT(ob) && !IS_MARKED(ob))
                        markChildren(ob);
                }
            } else {
                TRACE_GC(("Array object @0x%x class is %s  - Not Scanning...\n", ob, cb->name));
            }
        } else {
            FieldBlock *fb;
            int i;

            TRACE_GC(("Scanning object @0x%x class is %s\n", ob, cb->name));

            /* Scan fieldblocks in current class and all super-classes
               and mark all object refs */

            for(;;) {
                fb = cb->fields;

                TRACE_GC(("scanning fields of class %s\n", cb->name));

                for(i = 0; i < cb->fields_count; i++, fb++)
                    if(!(fb->access_flags & ACC_STATIC) &&
                        ((*fb->type == 'L') || (*fb->type == '['))) {
                            Object *ob = (Object *)body[fb->offset];
                            TRACE_GC(("Field %s %s is an Object ref\n", fb->name, fb->type));
                            TRACE_GC(("Object @0x%x is valid %d\n", ob, IS_OBJECT(ob)));

                            if(IS_OBJECT(ob) && !IS_MARKED(ob))
                                markChildren(ob);
                    }
                class = cb->super;
                if(class == NULL)
                    break;
                cb = CLASS_CB(class); 
            }
        }
    }
}


/* Routines to retrieve snapshot of heap status */

int freeHeapMem() {
    return heapfree;
}

int totalHeapMem() {
    return heaplimit-heapbase;
}

int maxHeapMem() {
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

/* The finalizer thread waits for notification
 * of new finalizers (by the thread doing gc)
 * and then runs them */

void finalizerThreadLoop(Thread *self) {
    disableSuspend0(self, &self);

    for(;;) {
        lockVMWaitLock(run_fnlzr_lock, self);
        waitVMWaitLock(run_fnlzr_lock, self);
        unlockVMWaitLock(run_fnlzr_lock, self);
        runFinalizers();
    }
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

    /* Create and start VM threads for the async gc and finalizer */
    createVMThread("Finalizer", finalizerThreadLoop);

    if(!noasyncgc)
        createVMThread("Async GC", asyncGCThreadLoop);
}

/* Object allocation routines */

#define ADD_FINALIZED_OBJECT(ob)                                                   \
{                                                                                  \
    Thread *self;                                                                  \
    disableSuspend(self = threadSelf());                                           \
    lockVMLock(has_fnlzr_lock, self);                                              \
    TRACE_FNLZ(("Object @0x%x type %s has a finalize method...\n",                 \
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
    int size = cb->object_size * 4;
    Object *ob = (Object *)gcMalloc(size+sizeof(Object));

    if(ob != NULL) {
        ob->class = class;

        if(cb->finalizer != NULL)
            ADD_FINALIZED_OBJECT(ob);

        TRACE_ALLOC(("<ALLOC: allocated %s object @ 0x%x>\n", cb->name, ob));
    }

    return ob;
}
    
Object *allocArray(Class *class, int size, int el_size) {
    Object *ob;

    if(size > (INT_MAX - 4 - sizeof(Object)) / el_size) {
        signalException("java/lang/OutOfMemoryError", NULL);
        return NULL;
    }

    ob = (Object *)gcMalloc(size * el_size + 4 + sizeof(Object));

    if(ob != NULL) {
        *INST_DATA(ob) = size;
        ob->class = class;
        TRACE_ALLOC(("<ALLOC: allocated %s array object @ 0x%x>\n", CLASS_CB(class)->name, ob));
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
            class = findArrayClass("[Z");
            el_size = 1;
            break;

        case T_BYTE:
            class = findArrayClass("[B");
            el_size = 1;
            break;

        case T_CHAR:
            class = findArrayClass("[C");
            el_size = 2;
            break;

        case T_SHORT:
            class = findArrayClass("[S");
            el_size = 2;
            break;

        case T_INT:
            class = findArrayClass("[I");
            el_size = 4;
            break;

        case T_FLOAT:
            class = findArrayClass("[F");
            el_size = 4;
            break;

        case T_DOUBLE:
            class = findArrayClass("[D");
            el_size = 8;
            break;

        case T_LONG:
            class = findArrayClass("[J");
            el_size = 8;
            break;

        default:
            printf("Invalid array type %d - aborting VM...\n", type);
            exit(0);
    }

    return allocArray(class, size, el_size);
}

Object *allocMultiArray(Class *array_class, int dim, int *count) {

    int i;
    Object *array;

    if(dim > 1) {

        Class *aclass = findArrayClassFromClass(CLASS_CB(array_class)->name+1, array_class);
        array = allocArray(array_class, *count, 4);

        if(array == NULL)
            return NULL;

        for(i = 1; i <= *count; i++)
            if((INST_DATA(array)[i] = (u4)allocMultiArray(aclass, dim-1, count+1)) == 0)
                return NULL;
    } else {
        int el_size;

        switch(CLASS_CB(array_class)->name[1]) {
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
            case 'L':
                el_size = 4;
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
    TRACE_ALLOC(("<ALLOC: allocated class object @ 0x%x>\n", class));
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

        if(CLASS_CB(clone->class)->finalizer != NULL)
            ADD_FINALIZED_OBJECT(clone);

        TRACE_ALLOC(("<ALLOC: cloned object @ 0x%x clone @ 0x:%x>\n", ob, clone));
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
