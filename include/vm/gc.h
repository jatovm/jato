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
	int (*gc_register_finalizer)(struct vm_object *object, finalizer_fn finalizer);
	void (*gc_setup_signals)(void);
};

void gc_setup_boehm(void);

void gc_init(void);

extern struct gc_operations gc_ops;

/*
 * There are three families of functions for memory allocation in
 * the VM. They differ in the way allocated memory region is treated
 * by garbage collector:
 *
 * malloc()     Allocated memory region is not collected by GC and its
 * zalloc()     content is not scanned for object references. If it
 * free()       contains any object references then those references
 *              will not prevent objects from being collected. Special
 *              care must be taken when putting object references in
 *              such regions because those objects may be collected
 *              and their memory may be allocated to another object
 *              without notice. If object reference must be stored in
 *              such regions one should consider using the
 *              vm_reference API (see include/vm/reference.h). There
 *              are some situations when it is valid to store object
 *              reference directly, for example reference to
 *              java.lang.VMThread in field "vmthread" in struct
 *              vm_thread. In this situation the lifetime of struct
 *              vm_thread is bound by the lifetime of object stored
 *              in "vmthread" so it is always valid.
 *
 * vm_alloc()   Allocates uncollectable memory region. The content is
 * vm_zalloc()  scanned for references by GC, so any object references
 * vm_free()    put in that region prevent collection of that object.
 *              Use this function to allocate VM structures with
 *              strong object references. If the memory region is
 *              large and contains only few object references
 *              then one should consider using malloc() family and
 *              vm_reference API to manage object references. This
 *              can increase garbage collection performance as the GC
 *              scans whole region allocated with vm_alloc() for object
 *              references.
 *
 * gc_alloc()   Allocates collectable memory region. Can not be freed
 *              manually. The content is scanned for object references.
 *              This is used to allocate Java objects.
 */

static inline void *gc_alloc(size_t size)
{
	return gc_ops.gc_alloc(size);
}

static inline void *vm_alloc(size_t size)
{
	return gc_ops.vm_alloc(size);
}

void *vm_zalloc(size_t size);

static inline void vm_free(void *ptr)
{
	gc_ops.vm_free(ptr);
}

static inline int
gc_register_finalizer(struct vm_object *object, finalizer_fn finalizer)
{
	return gc_ops.gc_register_finalizer(object, finalizer);
}

static inline void
gc_setup_signals(void)
{
	if (gc_ops.gc_setup_signals)
		gc_ops.gc_setup_signals();
}

void gc_safepoint(struct register_state *);
void suspend_handler(int, siginfo_t *, void *);
void wakeup_handler(int, siginfo_t *, void *);

#endif
