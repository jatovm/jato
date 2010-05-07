#ifndef VM_GC_H
#define VM_GC_H

#include <stdbool.h>
#include <signal.h>

#include "vm/object.h"

struct register_state;

extern unsigned long max_heap_size;
extern void *gc_safepoint_page;
extern bool verbose_gc;
extern int dont_gc;

void gc_init(void);

void *gc_alloc(size_t size);

/*
 * Always use this function to allocate memory region which directly
 * contains references to collectable objects.  Othervise such object
 * might be prematurely collected if the only reference to the object
 * is inside this region.
 */
void *vm_alloc(size_t size);
void *vm_zalloc(size_t size);

/*
 * Use this function to free memory allocated with vm_alloc()
 */
void vm_free(void *ptr);

void gc_safepoint(struct register_state *);
void suspend_handler(int, siginfo_t *, void *);
void wakeup_handler(int, siginfo_t *, void *);

typedef void (*finalizer_fn)(struct vm_object *object, void *param);

void gc_register_finalizer(struct vm_object *obj, finalizer_fn finalizer, void *param);

#endif
