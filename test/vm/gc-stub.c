#include "vm/stdlib.h"
#include "vm/gc.h"

#include <stdlib.h>

void *gc_safepoint_page;

void *do_gc_alloc(size_t size)
{
	return zalloc(size);
}

void gc_attach_thread(void)
{
}

void gc_detach_thread(void)
{
}

void *do_vm_alloc(size_t size)
{
	return malloc(size);
}

void *vm_zalloc(size_t size)
{
	void *result;

	result = vm_alloc(size);
	memset(result, 0, size);
	return result;
}

void do_vm_free(void *ptr)
{
	free(ptr);
}

struct gc_operations gc_ops = {
	.gc_alloc		= do_gc_alloc,
	.vm_alloc		= do_vm_alloc,
	.vm_free		= do_vm_free,
};
