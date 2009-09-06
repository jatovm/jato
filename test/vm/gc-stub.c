#include "vm/stdlib.h"
#include "vm/gc.h"

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
