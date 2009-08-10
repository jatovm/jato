#include "lib/guard-page.h"
#include "vm/die.h"
#include "vm/gc.h"

void *gc_safepoint_page;

void gc_init(void)
{
	gc_safepoint_page = alloc_guard_page();
	if (!gc_safepoint_page)
		die("Couldn't allocate GC safepoint guard page");
}
