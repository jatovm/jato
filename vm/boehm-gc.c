#include "vm/gc.h"

#include "../boehmgc/include/gc.h"

#include <stdio.h>

static void *gc_out_of_memory(size_t nr)
{
	if (verbose_gc)
		fprintf(stderr, "[GC]\n");

	GC_gcollect();

	return NULL;	/* is this ok? */
}

static void gc_ignore_warnings(char *msg, GC_word arg)
{
}

static int
do_gc_register_finalizer(struct vm_object *object, finalizer_fn finalizer)
{
	GC_register_finalizer_no_order(object, (GC_finalization_proc) finalizer,
				       NULL, NULL, NULL);
	return 0;
}

static void *do_gc_malloc(size_t size)
{
	void *p;

	p = GC_malloc(size);
	if (!p)
		return NULL;

	memset(p, 0, size);

	return p;
}

static void *do_gc_malloc_noscan(size_t size)
{
	void *p;

	p = GC_malloc_atomic(size);
	if (!p)
		return NULL;

	memset(p, 0, size);

	return p;
}

static void *do_gc_malloc_uncollectable(size_t size)
{
	void *p;

	p = GC_malloc_uncollectable(size);
	if (!p)
		return NULL;

	memset(p, 0, size);

	return p;
}

static void do_gc_free(void *ptr)
{
	GC_free(ptr);
}

void gc_setup_boehm(void)
{
	gc_ops		= (struct gc_operations) {
		.gc_alloc		= do_gc_malloc,
		.gc_alloc_noscan	= do_gc_malloc_noscan,
		.vm_alloc		= do_gc_malloc_uncollectable,
		.vm_free		= do_gc_free,
		.gc_register_finalizer	= do_gc_register_finalizer
	};

	GC_set_warn_proc(gc_ignore_warnings);

	GC_oom_fn	= gc_out_of_memory;

	GC_quiet	= 1;

	GC_dont_gc	= dont_gc;

	GC_INIT();

	GC_set_max_heap_size(max_heap_size);
}
