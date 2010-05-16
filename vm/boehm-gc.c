#include "vm/gc.h"

#include "../boehmgc/include/gc.h"

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

void gc_setup_boehm(void)
{
	gc_ops		= (struct gc_operations) {
		.gc_alloc	= GC_malloc,
		.vm_alloc	= GC_malloc_uncollectable,
		.vm_free	= GC_free,
	};

	GC_set_warn_proc(gc_ignore_warnings);

	GC_oom_fn	= gc_out_of_memory;

	GC_quiet	= 1;

	GC_dont_gc	= dont_gc;

	GC_INIT();

	GC_set_max_heap_size(max_heap_size);
}
