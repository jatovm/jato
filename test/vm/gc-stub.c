#include "vm/stdlib.h"
#include "vm/gc.h"

#include <stdlib.h>

void *gc_safepoint_page;

void *gc_alloc(size_t size)
{
	return zalloc(size);
}

void gc_attach_thread(void)
{
}

void gc_detach_thread(void)
{
}

void *vm_alloc(size_t size)
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

void vm_free(void *ptr)
{
	free(ptr);
}

void gc_register_finalizer(struct vm_object *object, finalizer_fn finalizer, void *param)
{
}
