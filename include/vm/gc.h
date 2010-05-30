#ifndef VM_GC_H
#define VM_GC_H

#include <stdbool.h>
#include <signal.h>

#include "vm/object.h"

struct register_state;

extern unsigned long		max_heap_size;
extern void			*gc_safepoint_page;
extern bool			newgc_enabled;
extern bool			verbose_gc;
extern int			dont_gc;

typedef void (*finalizer_fn)(struct vm_object *object);

struct gc_operations {
	void *(*gc_alloc)(size_t size);
	void *(*vm_alloc)(size_t size);
	void (*vm_free)(void *p);
	int (*gc_register_finalizer)(struct vm_object *object,
				     finalizer_fn finalizer);
};

void gc_setup_boehm(void);

void gc_init(void);

extern struct gc_operations gc_ops;

static inline void *gc_alloc(size_t size)
{
	return gc_ops.gc_alloc(size);
}

/*
 * Always use this function to allocate memory region which directly
 * contains references to collectable objects.  Othervise such object
 * might be prematurely collected if the only reference to the object
 * is inside this region.
 */
static inline void *vm_alloc(size_t size)
{
	return gc_ops.vm_alloc(size);
}

void *vm_zalloc(size_t size);

/*
 * Use this function to free memory allocated with vm_alloc()
 */
static inline void vm_free(void *ptr)
{
	gc_ops.vm_free(ptr);
}

static inline int
gc_register_finalizer(struct vm_object *object, finalizer_fn finalizer)
{
	return gc_ops.gc_register_finalizer(object, finalizer);
}

void gc_safepoint(struct register_state *);
void suspend_handler(int, siginfo_t *, void *);
void wakeup_handler(int, siginfo_t *, void *);

#endif
