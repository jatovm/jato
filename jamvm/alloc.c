/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007
 * Robert Lougher <rob@lougher.org.uk>.
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
#include "lock.h"

/* Trace GC heap mark/sweep phases - useful for debugging heap
 * corruption */
#ifdef TRACEGC
#define TRACE_GC(fmt, ...) jam_printf(fmt, ## __VA_ARGS__)
#else
#define TRACE_GC(fmt, ...)
#endif

/* Trace GC Compaction phase */
#ifdef TRACECOMPACT
#define TRACE_COMPACT(fmt, ...) jam_printf(fmt, ## __VA_ARGS__)
#else
#define TRACE_COMPACT(fmt, ...)
#endif

/* Trace class, object and array allocation */
#ifdef TRACEALLOC
#define TRACE_ALLOC(fmt, ...) jam_printf(fmt, ## __VA_ARGS__)
#else
#define TRACE_ALLOC(fmt, ...)
#endif

/* Trace object finalization */
#ifdef TRACEFNLZ
#define TRACE_FNLZ(fmt, ...) jam_printf(fmt, ## __VA_ARGS__)
#else
#define TRACE_FNLZ(fmt, ...)
#endif

/* Object alignment */
#define OBJECT_GRAIN            8

/* Bits used within the chunk header (see also alloc.h) */
#define ALLOC_BIT               1
#define SPECIAL_BIT             4
#define HAS_HASHCODE_BIT        (1<<31)
#define HASHCODE_TAKEN_BIT      (1<<30)

#define HDR_FLAGS_MASK          ~(ALLOC_BIT|FLC_BIT|SPECIAL_BIT| \
                                  HAS_HASHCODE_BIT|HASHCODE_TAKEN_BIT)

/* Macros for getting values from the chunk header */
#define HEADER(ptr)             *((uintptr_t*)ptr)
#define HDR_SIZE(hdr)           (hdr & HDR_FLAGS_MASK)
#define HDR_ALLOCED(hdr)        (hdr & ALLOC_BIT)
#define HDR_THREADED(hdr)       ((hdr & (ALLOC_BIT|FLC_BIT)) == FLC_BIT)
#define HDR_SPECIAL_OBJ(hdr)    (hdr & SPECIAL_BIT)
#define HDR_HASHCODE_TAKEN(hdr) (hdr & HASHCODE_TAKEN_BIT)
#define HDR_HAS_HASHCODE(hdr)   (hdr & HAS_HASHCODE_BIT)

/* Macro to mark an object as "special" by setting the special
   bit in the block header.  These are treated differently by GC */
#define SET_SPECIAL_OB(ob) {               \
    uintptr_t *hdr_addr = HDR_ADDRESS(ob); \
    *hdr_addr |= SPECIAL_BIT;              \
}

/* 1 word header format
  31                                       210
   -------------------------------------------
  |              block size               |   |
   -------------------------------------------
   ^ has hashcode bit                        ^ alloc bit
    ^ hashcode taken bit                    ^ flc bit
                                           ^ special bit
*/

static int verbosegc;
static int compact_override;
static int compact_value;

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

/* Compaction needs to know which object references are
   conservative (i.e. looks like a reference).  The objects
   can't be moved in case they aren't really references. */
static Object **conservative_roots = NULL;
static int conservative_root_count = 0;

/* Above list is transformed into a hashtable before compaction */
static uintptr_t *con_roots_hashtable;
static int con_roots_hashtable_size;

/* List holding object references from outside the heap
   which have been registered with the GC.  Unregistered
   references (outside of known structures) will not be
   scanned or threaded during GC/Compaction */
static Object ***registered_refs = NULL;
static int registered_refs_count = 0;

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
static VMLock registered_refs_lock;
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
#define MARK(ptr,mark)     markBits[MARKENTRY(ptr)]|=mark<<MARKOFFSET(ptr);

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

    TRACE_GC("Allocated mark bits - size is %d\n", markBitSize);
}

void clearMarkBits() {
    memset(markBits, 0, markBitSize*sizeof(*markBits));
}

void initialiseAlloc(InitArgs *args) {

#ifdef USE_MALLOC
    /* Don't use mmap - malloc max heap size */
    char *mem = (char*)malloc(args->max_heap);
    min = max;
    if(mem == NULL) {
#else
    char *mem = (char*)mmap(0, args->max_heap, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0);
    if(mem == MAP_FAILED) {
#endif
        perror("Aborting the VM -- couldn't allocate the heap");
        exitVM(1);
    }

    /* Align heapbase so that start of heap + HEADER_SIZE is object aligned */
    heapbase = (char*)(((uintptr_t)mem+HEADER_SIZE+OBJECT_GRAIN-1)&~(OBJECT_GRAIN-1))-HEADER_SIZE;

    /* Ensure size of heap is multiple of OBJECT_GRAIN */
    heaplimit = heapbase+((args->min_heap-(heapbase-mem))&~(OBJECT_GRAIN-1));

    heapmax = heapbase+((args->max_heap-(heapbase-mem))&~(OBJECT_GRAIN-1));

    freelist = (Chunk*)heapbase;
    freelist->header = heapfree = heaplimit-heapbase;
    freelist->next = NULL;

    TRACE_GC("Alloced heap size %p\n", heaplimit-heapbase);
    allocMarkBits();

    initVMLock(heap_lock);
    initVMLock(has_fnlzr_lock);
    initVMLock(registered_refs_lock);
    initVMWaitLock(run_finaliser_lock);
    initVMWaitLock(reference_lock);

    verbosegc = args->verbosegc;
}

/* ------------------------- MARK PHASE ------------------------- */

/* Forward declaration */
void markChildren(Object *ob, int mark, int mark_soft_refs);

int isMarked(Object *ob) {
    return ob != NULL && IS_MARKED(ob);
}

void markObject(Object *object, int mark, int mark_soft_refs) {
    if(object != NULL && mark > IS_MARKED(object))
        markChildren(object, mark, mark_soft_refs);
}

void markRoot(Object *object) {
    if(object != NULL)
        MARK(object, HARD_MARK);
}

void markConservativeRoot(Object *object) {
    if(object == NULL)
        return;

    MARK(object, HARD_MARK);

    if((conservative_root_count % LIST_INCREMENT) == 0) {
        int new_size = conservative_root_count + LIST_INCREMENT;
        conservative_roots = sysRealloc(conservative_roots,
                                        new_size * sizeof(Object *));
    }
    conservative_roots[conservative_root_count++] = object;
}

void freeConservativeRoots() {
    free(conservative_roots);
    conservative_roots = NULL;
    conservative_root_count = 0;
}

void scanThread(Thread *thread) {
    ExecEnv *ee = thread->ee;
    Frame *frame = ee->last_frame;
    uintptr_t *end, *slot;

    TRACE_GC("Scanning stacks for thread 0x%x id %d\n", thread, thread->id);

    /* Mark the java.lang.Thread object */
    markConservativeRoot(ee->thread);

    /* Mark any pending exception raised on this thread */
    markConservativeRoot(ee->exception);

    /* Scan the thread's C stack and mark all references */
    slot = (uintptr_t*)getStackTop(thread);
    end = (uintptr_t*)getStackBase(thread);

    for(; slot < end; slot++)
        if(IS_OBJECT(*slot)) {
            Object *ob = (Object*)*slot;
            TRACE_GC("Found C stack ref @%p object ref is %p\n", slot, ob);
            markConservativeRoot(ob);
        }

    /* Scan the thread's Java stack and mark all references */
    slot = frame->ostack + frame->mb->max_stack;

    while(frame->prev != NULL) {
        if(frame->mb != NULL) {
            TRACE_GC("Scanning %s.%s\n", CLASS_CB(frame->mb->class)->name, frame->mb->name);
            TRACE_GC("lvars @%p ostack @%p\n", frame->lvars, frame->ostack);

            /* Mark the method's defining class.  This should always
               be reachable otherwise, but mark to be safe */
            markConservativeRoot((Object*)frame->mb->class);
        }

        end = frame->ostack;

        for(; slot >= end; slot--)
            if(IS_OBJECT(*slot)) {
                Object *ob = (Object*)*slot;
                TRACE_GC("Found Java stack ref @%p object ref is %p\n", slot, ob);
                markConservativeRoot(ob);
            }

        slot -= sizeof(Frame)/sizeof(uintptr_t);
        frame = frame->prev;
    }
}

void markClassData(Class *class, int mark, int mark_soft_refs) {
    ClassBlock *cb = CLASS_CB(class);
    ConstantPool *cp = &cb->constant_pool;
    FieldBlock *fb = cb->fields;
    int i;

    TRACE_GC("Marking class %s\n", cb->name);

    /* Recursively mark the class's classloader */
    if(cb->class_loader != NULL && mark > IS_MARKED(cb->class_loader))
        markChildren(cb->class_loader, mark, mark_soft_refs);

    TRACE_GC("Marking static fields for class %s\n", cb->name);

    /* Static fields are initialised to default values during
       preparation (done in the link phase).  Therefore, don't
       scan if the class hasn't been linked */
    if(cb->state >= CLASS_LINKED)
        for(i = 0; i < cb->fields_count; i++, fb++)
            if((fb->access_flags & ACC_STATIC) &&
                        ((*fb->type == 'L') || (*fb->type == '['))) {
                Object *ob = (Object *)fb->static_value;
                TRACE_GC("Field %s %s object @%p\n", fb->name, fb->type, ob);
                if(ob != NULL && mark > IS_MARKED(ob))
                    markChildren(ob, mark, mark_soft_refs);
            }

    TRACE_GC("Marking constant pool resolved strings for class %s\n", cb->name);

    /* Scan the constant pool and mark all resolved string references */
    for(i = 1; i < cb->constant_pool_count; i++)
        if(CP_TYPE(cp, i) == CONSTANT_ResolvedString) {
            Object *string = (Object *)CP_INFO(cp, i);
            TRACE_GC("Resolved String @ constant pool idx %d @%p\n", i, string);
            if(mark > IS_MARKED(string))
                markChildren(string, mark, mark_soft_refs);
        }
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
            TRACE_GC("Scanning Array object @%p class is %s len is %d\n", ob, cb->name, len);

            for(i = 0; i < len; i++) {
                Object *ob = *body++;
                TRACE_GC("Object at index %d is @%p\n", i, ob);

                if(ob != NULL && mark > IS_MARKED(ob))
                    markChildren(ob, mark, mark_soft_refs);
            }
        } else {
            TRACE_GC("Array object @%p class is %s  - Not Scanning...\n", ob, cb->name);
        }
    } else {
        uintptr_t *body = INST_DATA(ob);
        int i;

        if(IS_CLASS_CLASS(cb)) {
            TRACE_GC("Found class object @%p name is %s\n", ob, CLASS_CB(ob)->name);
            markClassData((Class*)ob, mark, mark_soft_refs);
        } else
            if(IS_CLASS_LOADER(cb)) {
                TRACE_GC("Mark found class loader object @%p class %s\n", ob, cb->name);
                markLoaderClasses(ob, mark, mark_soft_refs);
            } else
                if(IS_VMTHROWABLE(cb)) {
                    TRACE_GC("Mark found VMThrowable object @%p\n", ob);
                    markVMThrowable(ob, mark, mark_soft_refs);
                } else
                    if(IS_REFERENCE(cb)) {
                        Object *referent = (Object *)body[ref_referent_offset];

                        TRACE_GC("Mark found Reference object @%p class %s flags %d referent %p\n",
                                 ob, cb->name, cb->flags, referent);

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
                                TRACE_GC("Marking referent object @%p mark %d ref_mark %d new_mark %d\n",
                                                                        referent, mark, ref_mark, new_mark);
                                markChildren(referent, new_mark, mark_soft_refs);
                            }
                        }
                    }

        TRACE_GC("Scanning object @%p class is %s\n", ob, cb->name);

        /* The reference offsets table consists of a list of start and
           end offsets corresponding to the references within the object's
           instance data.  Scan the list, and mark all references. */

        for(i = 0; i < cb->refs_offsets_size; i++) {
            int offset = cb->refs_offsets_table[i].start;
            int end = cb->refs_offsets_table[i].end;

            for(; offset < end; offset++) {
                Object *ob = (Object *)body[offset];
                TRACE_GC("Offset %d reference @%p\n", offset, ob);

                if(ob != NULL && mark > IS_MARKED(ob))
                    markChildren(ob, mark, mark_soft_refs);
            }
        }
    }
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
        for(i = list##_start; i < list##_end; i++) {                     \
            action(list##_list[i]);                                      \
    } else {                                                             \
        for(i = list##_start; i < list##_size; i++) {                    \
            action(list##_list[i]);                                      \
        }                                                                \
        for(i = 0; i < list##_end; i++) {                                \
            action(list##_list[i]);                                      \
        }                                                                \
    }                                                                    \
}

static void doMark(Thread *self, int mark_soft_refs) {
    char *ptr;
    int i, j;

    clearMarkBits();

    if(oom) MARK(oom, HARD_MARK);
    markBootClasses();
    markJNIGlobalRefs();
    scanThreads();

    /* All roots should now be marked.  Scan the heap and recursively
       mark all marked objects - once the heap has been scanned all
       reachable objects should be marked */

    for(ptr = heapbase; ptr < heaplimit;) {
        uintptr_t hdr = HEADER(ptr);
        uintptr_t size = HDR_SIZE(hdr);

#ifdef DEBUG
        jam_printf("Block @%p size %d alloced %d\n", ptr, size, HDR_ALLOCED(hdr));
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

    /* There may be references still waiting to be enqueued by the
       reference handler (from a previous GC).  Remove them if
       they're now unreachable as they will be collected */

#define CLEAR_UNMARKED(element) \
         if(element && !IS_MARKED(element)) element = NULL

    ITERATE_OBJECT_LIST(reference, CLEAR_UNMARKED);

    /* Scan the interned string hash table and remove
       any entries that are unmarked */
    freeInternedStrings();
}

/* ------------------------- SWEEP PHASE ------------------------- */

int handleMarkedSpecial(Object *ob) {
    ClassBlock *cb = CLASS_CB(ob->class);
    int cleared = FALSE;

    if(IS_REFERENCE(cb)) {
        Object *referent = (Object *)INST_DATA(ob)[ref_referent_offset];

        if(referent != NULL) {
            int ref_mark = IS_MARKED(referent);

            TRACE_GC("FREE: found Reference Object @%p class %s flags %d referent %x mark %d\n",
                      ob, cb->name, cb->flags, referent, ref_mark);

            if(IS_PHANTOM_REFERENCE(cb)) {
                if(ref_mark != PHANTOM_MARK)
                    goto out;
            } else {
                if(ref_mark == HARD_MARK)
                    goto out;

                TRACE_GC("FREE: Clearing the referent field.\n");
                INST_DATA(ob)[ref_referent_offset] = 0;
                cleared = TRUE;
            }

            /* If the reference has a queue, add it to the list for enqueuing
               by the Reference Handler thread. */

            if((Object *)INST_DATA(ob)[ref_queue_offset] != NULL) {
                TRACE_GC("FREE: Adding to list for enqueuing.\n");
                ADD_TO_OBJECT_LIST(reference, ob);
            }
        }
    }
out:
    return cleared;
}

void handleUnmarkedSpecial(Object *ob) {
    if(IS_CLASS(ob)) {
        if(verbosegc) {
            ClassBlock *cb = CLASS_CB(ob);
            if(!IS_CLASS_DUP(cb))
                jam_printf("<GC: Unloading class %s>\n", cb->name);
        }
        freeClassData(ob);
    } else
        if(IS_CLASS_LOADER(CLASS_CB(ob->class))) {
            TRACE_GC("FREE: Freeing class loader object %p\n", ob);
            unloadClassLoaderDlls(ob);
            freeClassLoaderData(ob);
        } else
            if(IS_VMTHREAD(CLASS_CB(ob->class))) {
                /* Free the native thread structure (see comment
                   in detachThread (thread.c) */
                TRACE_GC("FREE: Freeing native thread for VMThread object %p\n", ob);
                free(threadSelf0(ob));
            }
}

static uintptr_t doSweep(Thread *self) {
    char *ptr;
    Chunk newlist;
    Chunk *curr = NULL, *last = &newlist;

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

            if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL)
                handleUnmarkedSpecial(ob);

            TRACE_GC("FREE: Freeing ob @%p class %s - start of block\n", ob,
                                    ob->class ? CLASS_CB(ob->class)->name : "?");
        }
        else
            TRACE_GC("FREE: Unalloced block @%p size %d - start of block\n", ptr, size);
        
        curr = (Chunk *) ptr;

        /* Clear any set flag bits within the header */
        curr->header &= HDR_FLAGS_MASK;

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

                if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL)
                    handleUnmarkedSpecial(ob);

                TRACE_GC("FREE: Freeing object @%p class %s - merging onto block @%p\n",
                                       ob, ob->class ? CLASS_CB(ob->class)->name : "?", curr);

            }
            else 
                TRACE_GC("FREE: unalloced block @%p size %d - merging onto block @%p\n", ptr, size, curr);
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

        if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL && handleMarkedSpecial(ob))
            cleared++;

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
        jam_printf("Chunk @%p size: %d\n", c, c->header);
}
#endif
 
    if(verbosegc) {
        long long size = heaplimit-heapbase;
        long long pcnt_used = ((long long)heapfree)*100/size;
        jam_printf("<GC: Allocated objects: %lld>\n", (long long)marked);
        jam_printf("<GC: Freed %lld object(s) using %lld bytes",
			(long long)unmarked, (long long)freed);
        if(cleared)
            jam_printf(", cleared %lld reference(s)", (long long)cleared);
        jam_printf(">\n<GC: Largest block is %lld total free is %lld out of %lld (%lld%%)>\n",
                         (long long)largest, (long long)heapfree, size, pcnt_used);
    }

    /* Return the size of the largest free chunk in heap - this
       is the largest allocation request that can be satisfied */

    return largest;
}

/* ------------------------- COMPACT PHASE ------------------------- */

void threadReference(Object **ref) {
    Object *ob = *ref;
    uintptr_t *hdr = HDR_ADDRESS(ob);
    TRACE_COMPACT("Threading ref addr %p object ref %p link %p\n", ref, ob, *hdr);

    *ref = (Object*)*hdr;
    *hdr = ((uintptr_t)ref | FLC_BIT);
}

void unthreadHeader(uintptr_t *hdr_addr, Object *new_addr) {
    uintptr_t hdr = *hdr_addr;

    TRACE_COMPACT("Unthreading header address %p new addr %p\n", hdr_addr, new_addr);

    while(HDR_THREADED(hdr)) {
        uintptr_t *ref_addr = (uintptr_t*)(hdr & ~0x3);

        TRACE_COMPACT("updating ref address %p\n", ref_addr);
        hdr = *ref_addr;
        *ref_addr = (uintptr_t)new_addr;
    }

    TRACE_COMPACT("Replacing original header contents %p\n", hdr);
    *hdr_addr = hdr;
}

static void threadObjectLists() {
    int i;

    for(i = 0; i < has_finaliser_count; i++)
        threadReference(&has_finaliser_list[i]);

#define THREAD_REFS(element) \
         if(element) threadReference(&element)

    ITERATE_OBJECT_LIST(run_finaliser, THREAD_REFS);
    ITERATE_OBJECT_LIST(reference, THREAD_REFS);
}

void addConservativeRoots2Hash() {
    int i;

    for(i = 1; i < conservative_root_count; i <<= 1);
    con_roots_hashtable_size = i << 1;

    con_roots_hashtable = sysMalloc(con_roots_hashtable_size * sizeof(uintptr_t));
    memset(con_roots_hashtable, 0, con_roots_hashtable_size * sizeof(uintptr_t));

    for(i = 0; i < conservative_root_count; i++) {
        uintptr_t data = ((uintptr_t)conservative_roots[i]) >> LOG_BYTESPERMARK;
        int index = data & (con_roots_hashtable_size-1);

        TRACE_COMPACT("Adding conservative root %p\n", conservative_roots[i]);

        while(con_roots_hashtable[index] && con_roots_hashtable[index] != data)
            index = (index + 1) & (con_roots_hashtable_size-1);

        con_roots_hashtable[index] = data;
    }
}

void registerStaticObjectRefLocked(Object **ref) {
    Thread *self = threadSelf();

    disableSuspend(self);
    lockVMLock(registered_refs_lock, self);

    registerStaticObjectRef(ref);

    unlockVMLock(registered_refs_lock, self);
    enableSuspend(self);
}

void registerStaticObjectRef(Object **ref) {
    if((registered_refs_count % LIST_INCREMENT) == 0) {
        int new_size = registered_refs_count + LIST_INCREMENT;
        registered_refs = sysRealloc(registered_refs,
                                     new_size * sizeof(Object **));
    }
    registered_refs[registered_refs_count++] = ref;
}

void threadRegisteredReferences() {
    int i;

    for(i = 0; i < registered_refs_count; i++)
        if(*registered_refs[i] != NULL)
            threadReference(registered_refs[i]);
}

#define IS_CONSERVATIVE_ROOT(ob)                                            \
({                                                                          \
    uintptr_t data = ((uintptr_t)ob) >> LOG_BYTESPERMARK;                   \
    int index = data & (con_roots_hashtable_size-1);                        \
                                                                            \
    while(con_roots_hashtable[index] && con_roots_hashtable[index] != data) \
        index = (index + 1) & (con_roots_hashtable_size-1);                 \
    con_roots_hashtable[index];                                             \
})
 
#define ADD_CHUNK_TO_FREELIST(start, end)     \
{                                             \
    Chunk *curr = (Chunk *) start;            \
    curr->header = end - start;               \
                                              \
    if(curr->header >= MIN_OBJECT_SIZE) {     \
        last->next = curr;                    \
        last = curr;                          \
    }                                         \
                                              \
    if(curr->header > largest)                \
        largest = curr->header;               \
                                              \
    /* Add onto total count of free chunks */ \
    heapfree += curr->header;                 \
}

int compactSlideBlock(char *block_addr, char *new_addr) {
    uintptr_t hdr = HEADER(block_addr);
    uintptr_t size = HDR_SIZE(hdr);

    /* Slide the object down the heap.  Use memcpy if
       the areas don't overlap as it should be faster */
    if(new_addr + size <= block_addr)
        memcpy(new_addr, block_addr, size);
    else
        memmove(new_addr, block_addr, size);

    /* If the objects hashCode (address) has been taken we must
       maintain the same value after the object has been moved */
    if(HDR_HASHCODE_TAKEN(hdr)) {
        uintptr_t *hdr_addr = &HEADER(new_addr);
        uintptr_t *hash_addr = (uintptr_t*)(new_addr + size);

        TRACE_COMPACT("Adding hashCode to object %p\n", block_addr + HEADER_SIZE);

        /* Add the original address onto the end of the object */
        *hash_addr = (uintptr_t)(block_addr + HEADER_SIZE);
        *hdr_addr &= ~HASHCODE_TAKEN_BIT;
        *hdr_addr |= HAS_HASHCODE_BIT;
        *hdr_addr += OBJECT_GRAIN;
        return TRUE;
    }

    return FALSE;
}

void threadClassData(Class *class, Class *new_addr) {
    ClassBlock *cb = CLASS_CB(class);
    ConstantPool *cp = &cb->constant_pool;
    FieldBlock *fb = cb->fields;
    int i;

    TRACE_COMPACT("Threading class %s @%p\n", cb->name, class);

    if(cb->class_loader != NULL)
        threadReference(&cb->class_loader);

    if(cb->super != NULL)
        threadReference((Object**)&cb->super);

    for(i = 0; i < cb->interfaces_count; i++)
        if(cb->interfaces[i] != NULL)
            threadReference((Object**)&cb->interfaces[i]);

    if(IS_ARRAY(cb))
        threadReference((Object**)&cb->element_class);

    for(i = 0; i < cb->imethod_table_size; i++)
        threadReference((Object**)&cb->imethod_table[i].interface);

    TRACE_COMPACT("Threading static fields for class %s\n", cb->name);

    /* If the class has not been linked it's
       statics will not be initialised */
    if(cb->state >= CLASS_LINKED)
        for(i = 0; i < cb->fields_count; i++, fb++)
            if((fb->access_flags & ACC_STATIC) &&
                        ((*fb->type == 'L') || (*fb->type == '['))) {
                Object **ob = (Object **)&fb->static_value;
                TRACE_COMPACT("Field %s %s object @%p\n", fb->name, fb->type, *ob);
                if(*ob != NULL)
                    threadReference(ob);
            }

    TRACE_COMPACT("Threading constant pool references for class %s\n", cb->name);

    for(i = 1; i < cb->constant_pool_count; i++)
        if(CP_TYPE(cp, i) == CONSTANT_ResolvedClass || CP_TYPE(cp, i) == CONSTANT_ResolvedString) {
            TRACE_COMPACT("Constant pool ref idx %d type %d object @%p\n", i, CP_TYPE(cp, i), CP_INFO(cp, i));
            threadReference((Object**)&(CP_INFO(cp, i)));
        }

    /* Don't bother threading the references to the class from within the
       classes own method and field blocks.  As we know the new address we
       can update the address now. */

    for(i = 0; i < cb->fields_count; i++)
        cb->fields[i].class = new_addr;

    for(i = 0; i < cb->methods_count; i++)
        cb->methods[i].class = new_addr;
}

int threadChildren(Object *ob, Object *new_addr) {
    Class *class = ob->class;
    ClassBlock *cb = CLASS_CB(class);
    int cleared = FALSE;

    if(class == NULL)
        return FALSE;

    if(cb->name[0] == '[') {
        if((cb->name[1] == 'L') || (cb->name[1] == '[')) {
            Object **body = ARRAY_DATA(ob);
            int len = ARRAY_LEN(ob);
            int i;
            TRACE_COMPACT("Scanning Array object @%p class is %s len is %d\n", ob, cb->name, len);

            for(i = 0; i < len; i++, body++) {
                TRACE_COMPACT("Object at index %d is @%p\n", i, *body);

                if(*body != NULL)
                    threadReference(body);
            }
        } else {
            TRACE_COMPACT("Array object @%p class is %s - not Scanning...\n", ob, cb->name);
        }
    } else {
        Object **body = (Object**)INST_DATA(ob);
        int i;

        if(IS_CLASS_CLASS(cb)) {
            TRACE_COMPACT("Found class object @%p name is %s\n", ob, CLASS_CB(ob)->name);
            threadClassData((Class*)ob, (Class*)new_addr);
        } else
            if(IS_CLASS_LOADER(cb)) {
                TRACE_COMPACT("Found class loader object @%p class %s\n", ob, cb->name);
                threadLoaderClasses(ob);
            } else
                if(IS_REFERENCE(cb)) {
                    Object **referent = &body[ref_referent_offset];

                    if(*referent != NULL) {
                        int ref_mark = IS_MARKED(*referent);

                        TRACE_GC("Found Reference Object @%p class %s flags %d referent %x mark %d\n",
                                  ob, cb->name, cb->flags, *referent, ref_mark);

                        if(IS_PHANTOM_REFERENCE(cb)) {
                            if(ref_mark != PHANTOM_MARK)
                                goto out;
                        } else {
                            if(ref_mark == HARD_MARK)
                                goto out;

                            TRACE_GC("Clearing the referent field.\n");
                            *referent = 0;
                            cleared = TRUE;
                        }

                        /* If the reference has a queue, add it to the list for enqueuing
                           by the Reference Handler thread. */

                        if(body[ref_queue_offset] != NULL) {
                            TRACE_GC("Adding to list for enqueuing.\n");
                            ADD_TO_OBJECT_LIST(reference, new_addr);
                        }
out:
                        if(!cleared)
                            threadReference(referent);
                    }
                }

        TRACE_COMPACT("Scanning object @%p class is %s\n", ob, cb->name);

        /* The reference offsets table consists of a list of start and
           end offsets corresponding to the references within the object's
           instance data.  Scan the list, and thread all references. */

        for(i = 0; i < cb->refs_offsets_size; i++) {
            int offset = cb->refs_offsets_table[i].start;
            int end = cb->refs_offsets_table[i].end;

            for(; offset < end; offset++) {
                Object **ob = (Object **)&body[offset];
                TRACE_COMPACT("Offset %d reference @%p\n", offset, *ob);

                if(*ob != NULL)
                    threadReference(ob);
            }
        }
    }

    /* Finally thread the object's class reference */
    threadReference((Object**)&ob->class);

    return cleared;
}

uintptr_t doCompact() {
    char *ptr, *new_addr;
    Chunk newlist;
    Chunk *last = &newlist;

    /* Will hold the size of the largest free chunk
       after scanning */
    uintptr_t largest = 0;

    /* Variables used to store verbose gc info */
    uintptr_t marked = 0, unmarked = 0, freed = 0, cleared = 0, moved = 0;

    /* Amount of free heap is re-calculated during scan */
    heapfree = 0;

    /* Transform conservative root list into
       hash table for faster searching */
    addConservativeRoots2Hash();

    TRACE_COMPACT("COMPACT THREADING ROOTS\n");

    /* Thread object references from outside of the heap */
    threadObjectLists();
    threadRegisteredReferences();
    threadBootClasses();
    threadMonitorCache();
    threadInternedStrings();
    threadLiveClassLoaderDlls();

    TRACE_COMPACT("COMPACT PHASE ONE\n");

    /* First phase scans the heap, threads each objects references
       and updates forward references to each object */
    for(new_addr = ptr = heapbase; ptr < heaplimit;) {
        uintptr_t hdr = HEADER(ptr);
        uintptr_t size = HDR_SIZE(hdr);
        Object *ob;

        if(HDR_THREADED(hdr)) {
            ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_CONSERVATIVE_ROOT(ob))
                new_addr = ptr;

            /* Unthread forward references to the object */
            unthreadHeader((uintptr_t*)ptr, (Object*)(new_addr+HEADER_SIZE));

            /* Header is now unthreaded -- re-read it, and the size */
            hdr = HEADER(ptr);
            size = HDR_SIZE(hdr);

            goto marked_phase1;
        }

        if(HDR_ALLOCED(hdr)) {
            ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_MARKED(ob)) {
                if(IS_CONSERVATIVE_ROOT(ob))
                    new_addr = ptr;

marked_phase1:
                marked++;

                /* Thread references within the object */
                if(threadChildren(ob, (Object*)(new_addr+HEADER_SIZE)))
                    cleared++;

                if(new_addr != ptr && HDR_HASHCODE_TAKEN(hdr))
                    new_addr += OBJECT_GRAIN;

                new_addr += size;
                goto next;
            }

            if(HDR_SPECIAL_OBJ(hdr) && ob->class != NULL)
                handleUnmarkedSpecial(ob);

            freed += size;
            unmarked++;
        }

next:
        /* Skip to next block */
        ptr += size;
    }

    TRACE_COMPACT("COMPACT PHASE TWO\n");

    /* Second phase rescans the heap, updates backwards references
       to each object, and then moves them. */
    for(new_addr = ptr = heapbase; ptr < heaplimit;) {
        uintptr_t hdr = HEADER(ptr);
        uintptr_t size = HDR_SIZE(hdr);
        Object *ob;

        if(HDR_THREADED(hdr)) {
            ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_CONSERVATIVE_ROOT(ob) && new_addr != ptr) {
                ADD_CHUNK_TO_FREELIST(new_addr, ptr);
                new_addr = ptr;
            }

            /* Unthread backward references to the object */
            unthreadHeader((uintptr_t*)ptr, (Object*)(new_addr+HEADER_SIZE));

            /* Header is now unthreaded -- re-read */
            hdr = HEADER(ptr);
            size = HDR_SIZE(hdr);

            goto marked_phase2;
        }

        if(HDR_ALLOCED(hdr)) {
            ob = (Object*)(ptr+HEADER_SIZE);

            if(IS_MARKED(ob)) {
                if(IS_CONSERVATIVE_ROOT(ob) && new_addr != ptr) {
                    ADD_CHUNK_TO_FREELIST(new_addr, ptr);
                    new_addr = ptr;
                }

marked_phase2:
                /* Move the object to the new address */
                if(new_addr != ptr) {
                    TRACE_COMPACT("Moving object from %p to %p.\n", ob, new_addr+HEADER_SIZE);

                    if(compactSlideBlock(ptr, new_addr))
                        new_addr += OBJECT_GRAIN;

                    moved++;
                }

                new_addr += size;
            }
        }

        /* Skip to next block */
        ptr += size;
    }

    if(new_addr != heaplimit)
        ADD_CHUNK_TO_FREELIST(new_addr, heaplimit);

    /* We've now reconstructed the freelist, set freelist
       pointer to new list */
    last->next = NULL;
    freelist = newlist.next;

    /* Reset next allocation block to beginning of list */
    chunkpp = &freelist;

    /* Free conservative roots hash table */
    free(con_roots_hashtable);
    
    if(verbosegc) {
        long long size = heaplimit-heapbase;
        long long pcnt_used = ((long long)heapfree)*100/size;
        jam_printf("<GC: Allocated objects: %lld>\n", (long long)marked);
        jam_printf("<GC: Freed %lld object(s) using %lld bytes",
			(long long)unmarked, (long long)freed);
        if(cleared)
            jam_printf(", cleared %lld reference(s)", (long long)cleared);
        jam_printf(">\n<GC: Moved %lld objects, largest block is %lld total free is %lld out of %lld (%lld%%)>\n",
                         (long long)moved, (long long)largest, (long long)heapfree, size, pcnt_used);
    }

    /* Return the size of the largest free chunk in heap - this
       is the largest allocation request that can be satisfied */

    return largest;
}

void expandHeap(int min) {
    Chunk *chunk, *new;
    uintptr_t delta;

    if(verbosegc)
        jam_printf("<GC: Expanding heap - minimum needed is %d>\n", min);

    delta = (heaplimit-heapbase)/2;
    delta = delta < min ? min : delta;

    if((heaplimit + delta) > heapmax)
        delta = heapmax - heaplimit;

    /* Ensure new region is multiple of object grain in size */

    delta = (delta&~(OBJECT_GRAIN-1));

    if(verbosegc)
        jam_printf("<GC: Expanding heap by %lld bytes>\n", (long long)delta);

    new = (Chunk*)heaplimit;
    new->header = delta;
    new->next = NULL;

    if(freelist != NULL) {
    /* The freelist is in address order - find the last
       free chunk and add the new area to the end.  */

    for(chunk = freelist; chunk->next != NULL; chunk = chunk->next);
        chunk->next = new;
    } else
        freelist = new;

    heaplimit += delta;
    heapfree += delta;

    /* The heap has increased in size - need to reallocate
       the mark bits to cover new area */

    free(markBits);
    allocMarkBits();
}

/* ------------------------- GARBAGE COLLECT ------------------------- */

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

unsigned long gc0(int mark_soft_refs, int compact) {
    Thread *self = threadSelf();
    uintptr_t largest;

    /* Override compact if compaction has been specified
       on the command line */
    if(compact_override)
        compact = compact_value;

    /* Stop the world */
    disableSuspend(self);
    suspendAllThreads(self);

    /* Grab locks associated with the suspension blocked
       regions.  This ensures all threads have suspended
       or gone to sleep, and cannot modify a list or obtain
       a reference after the reference scans */

    /* Potential threads adding a newly created object */
    lockVMLock(has_fnlzr_lock, self);

    /* Held by the finaliser thread */
    lockVMWaitLock(run_finaliser_lock, self);

    /* Held by the reference handler thread */
    lockVMWaitLock(reference_lock, self);

    if(verbosegc) {
        struct timeval start;
        float mark_time;
        float scan_time;

        getTime(&start);
        doMark(self, mark_soft_refs);
        mark_time = endTime(&start)/1000000.0;

        getTime(&start);
        largest = compact ? doCompact() : doSweep(self);
        scan_time = endTime(&start)/1000000.0;

        jam_printf("<GC: Mark took %f seconds, %s took %f seconds>\n",
                                  mark_time, compact ? "compact" : "scan", scan_time);
    } else {
        doMark(self, mark_soft_refs);
        largest = compact ? doCompact() : doSweep(self);
    }

    /* Notify the reference handler.  If there's no
       work to be done it'll go back to sleep */
    notifyAllVMWaitLock(reference_lock, self);

    /* Restart the world */
    resumeAllThreads(self);
    enableSuspend(self);

    /* Release the locks */
    unlockVMLock(has_fnlzr_lock, self);
    unlockVMWaitLock(reference_lock, self);
    unlockVMWaitLock(run_finaliser_lock, self);

    freeConservativeRoots();

    return largest;
}

void gc1() {
    Thread *self;
    disableSuspend(self = threadSelf());
    lockVMLock(heap_lock, self);
    enableSuspend(self);
    gc0(TRUE, FALSE);
    unlockVMLock(heap_lock, self);
}

/* ------------------------- FINALISATION ------------------------- */

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


/* ------------------------- GC HELPER THREADS ------------------------- */

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
            jam_printf(verbose_message, diff > 0 ? diff : diff + list##_size);  \
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

void initialiseGC(InitArgs *args) {
    /* Pre-allocate an OutOfMemoryError exception object - we throw it
     * when we're really low on heap space, and can create FA... */

    MethodBlock *init;
    Class *oom_clazz = findSystemClass("java/lang/OutOfMemoryError");
    if(exceptionOccured()) {
        printException();
        exitVM(1);
    }

    /* Initialize it */
    init = lookupMethod(oom_clazz, "<init>", "(Ljava/lang/String;)V");
    oom = allocObject(oom_clazz);
    registerStaticObjectRef(&oom);

    executeMethod(oom, init, NULL);

    /* Create and start VM threads for the reference handler and finalizer */
    createVMThread("Finalizer", finalizerThreadLoop);
    createVMThread("Reference Handler", referenceHandlerThreadLoop);

    /* Create and start VM thread for asynchronous GC */
    if(!args->noasyncgc)
        createVMThread("Async GC", asyncGCThreadLoop);

    /* GC will use mark-sweep or mark-compact as appropriate, but this
       can be changed via the command line */
    compact_override = args->compact_specified;
    compact_value = args->do_compact;
}

/* ------------------------- ALLOCATION ROUTINES  ------------------------- */

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
            jam_printf("<GC: Alloc attempt for %d bytes failed.>\n", n);

        switch(state) {

            case gc:
                /* Normal failure.  Do a garbage-collection and retry
                   allocation if the largest block satisfies the request.
                   Attempt to ensure heap is at least 25% free, to stop
                   rapid gc cycles */
                largest = gc0(TRUE, FALSE);

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
                    jam_printf("<GC: Waiting for finalizers to be ran.>\n");

                runFinalizers0(self, 200);
                lockVMLock(heap_lock, self);
                enableSuspend(self);

                if(state != run_finalizers)
                    break;

                /* Retry gc, but this time compact the heap rather than just
                   sweeping it */
                largest = gc0(TRUE, TRUE);
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
                    jam_printf("<GC: Stack at maximum already.  Clearing Soft References>\n");

                /* Can't expand the heap any more.  Try GC again but this time
                   clearing all soft references.  Note we succeed if we can satisfy
                   the request -- we may have been able to all along, but with
                   nothing spare.  We may thrash, but it's better than throwing OOM */
                largest = gc0(FALSE, TRUE);
                if(n <= largest) {
                    state = gc;
                    break;
                }

                if(verbosegc)
                    jam_printf("<GC: completely out of heap space - throwing OutOfMemoryError>\n");

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
                    jam_printf("<GC: completely out of heap space - throwing prepared OutOfMemoryError>\n");

                state = gc;
                unlockVMLock(heap_lock, self);
                setException(oom);
                return NULL;
                break;
        }
    }

got_it:
#ifdef TRACEALLOC
    jam_printf("<ALLOC: took %d tries to find block.>\n", tries);
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

        if(IS_SPECIAL(cb))
            SET_SPECIAL_OB(ob);

        TRACE_ALLOC("<ALLOC: allocated %s object @%p>\n", cb->name, ob);
    }

    return ob;
}
    
Object *allocArray(Class *class, int size, int el_size) {
    Object *ob;

    /* Special check to protect against integer overflow */
    if(size > (INT_MAX - sizeof(u4) - sizeof(Object)) / el_size) {
        signalException("java/lang/OutOfMemoryError", NULL);
        return NULL;
    }

    ob = (Object *)gcMalloc(size * el_size + sizeof(u4) + sizeof(Object));

    if(ob != NULL) {
        ob->class = class;
        ARRAY_LEN(ob) = size;
        TRACE_ALLOC("<ALLOC: allocated %s array object @%p>\n", CLASS_CB(class)->name, ob);
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
            if(bool_array_class == NULL) {
                bool_array_class = findArrayClass("[Z");
                registerStaticClassRefLocked(&bool_array_class);
            }
            class = bool_array_class;
            el_size = 1;
            break;

        case T_BYTE:
            if(byte_array_class == NULL) {
                byte_array_class = findArrayClass("[B");
                registerStaticClassRefLocked(&byte_array_class);
            }
            class = byte_array_class;
            el_size = 1;
            break;

        case T_CHAR:
            if(char_array_class == NULL) {
                char_array_class = findArrayClass("[C");
                registerStaticClassRefLocked(&char_array_class);
            }
            class = char_array_class;
            el_size = 2;
            break;

        case T_SHORT:
            if(short_array_class == NULL) {
                short_array_class = findArrayClass("[S");
                registerStaticClassRefLocked(&short_array_class);
            }
            class = short_array_class;
            el_size = 2;
            break;

        case T_INT:
            if(int_array_class == NULL) {
                int_array_class = findArrayClass("[I");
                registerStaticClassRefLocked(&int_array_class);
            }
            class = int_array_class;
            el_size = 4;
            break;

        case T_FLOAT:
            if(float_array_class == NULL) {
                float_array_class = findArrayClass("[F");
                registerStaticClassRefLocked(&float_array_class);
            }
            class = float_array_class;
            el_size = 4;
            break;

        case T_DOUBLE:
            if(double_array_class == NULL) {
                double_array_class = findArrayClass("[D");
                registerStaticClassRefLocked(&double_array_class);
            }
            class = double_array_class;
            el_size = 8;
            break;

        case T_LONG:
            if(long_array_class == NULL) {
                long_array_class = findArrayClass("[J");
                registerStaticClassRefLocked(&long_array_class);
            }
            class = long_array_class;
            el_size = 8;
            break;

        default:
            jam_printf("Invalid array type %d - aborting VM...\n", type);
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
    SET_SPECIAL_OB(class);

    TRACE_ALLOC("<ALLOC: allocated class object @%p>\n", class);
    return class; 
}

Object *cloneObject(Object *ob) {
    uintptr_t hdr = *HDR_ADDRESS(ob);
    int size = HDR_SIZE(hdr)-HEADER_SIZE;
    Object *clone;

    /* If the object stores its original address the actual object
       data size will be smaller than the header size */
    if(HDR_HAS_HASHCODE(hdr))
        size -= OBJECT_GRAIN;

    clone = (Object*)gcMalloc(size);

    if(clone != NULL) {
        memcpy(clone, ob, size);

        /* We will also have copied the objects lock word */
        clone->lock = 0;

        if(IS_FINALIZED(CLASS_CB(clone->class)))
            ADD_FINALIZED_OBJECT(clone);

        if(HDR_SPECIAL_OBJ(hdr)) {
            SET_SPECIAL_OB(clone);

            /* Safety.  If it's a classloader, clear native
               pointer to class table */
            if(IS_CLASS_LOADER(CLASS_CB(clone->class)))
                INST_DATA(clone)[ldr_vmdata_offset] = 0;
        }

        TRACE_ALLOC("<ALLOC: cloned object @%p clone @%p>\n", ob, clone);
    }

    return clone;
}

uintptr_t getObjectHashcode(Object *ob) {
    uintptr_t *hdr_addr = HDR_ADDRESS(ob);

    /* If the object has had its hashCode taken and then
       it has been moved it will store the original value
       (see compactSlideBlock) */
    if(HDR_HAS_HASHCODE(*hdr_addr)) {
        uintptr_t *hash_addr = (uintptr_t *)((char*)hdr_addr +
                               HDR_SIZE(*hdr_addr) - OBJECT_GRAIN);
        return *hash_addr;
    }
 
    /* Mark that the hashCode has been taken, in case
       compaction later moves it */
    *hdr_addr |= HASHCODE_TAKEN_BIT;
    return (uintptr_t) ob;
}


/* ------- Routines to retrieve snapshot of heap status -------- */

unsigned long freeHeapMem() {
    return heapfree;
}

unsigned long totalHeapMem() {
    return heaplimit-heapbase;
}

unsigned long maxHeapMem() {
    return heapmax-heapbase;
}

/* ------ Allocation from system heap ------- */

void *sysMalloc(int n) {
    void *mem = malloc(n);

    if(mem == NULL && n > 0) {
        jam_fprintf(stderr, "Malloc failed - aborting VM...\n");
        exitVM(1);
    }

    return mem;
}

void *sysRealloc(void *ptr, int n) {
    void *mem = realloc(ptr, n);

    if(mem == NULL) {
        jam_fprintf(stderr, "Realloc failed - aborting VM...\n");
        exitVM(1);
    }

    return mem;
}

