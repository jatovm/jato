#include "jit/exception.h"

#include "vm/object.h"

#include <assert.h>

void throw_from_trampoline(void *ctx, struct vm_object *exception)
{
	assert(!"not implemented");
}
