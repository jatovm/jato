#include "arch/stack-frame.h"

#include "jit/expression.h"
#include "vm/method.h"

#include <assert.h>

unsigned long frame_local_offset(struct vm_method *vm_method, struct expression *expression)
{
	assert(!"not implemented");
}

unsigned long slot_offset(struct stack_slot *slot)
{
	assert(!"not implemented");
}

unsigned long slot_offset_64(struct stack_slot *slot)
{
	assert(!"not implemented");
}

unsigned long frame_locals_size(struct stack_frame *frame)
{
	assert(!"not implemented");
}

unsigned long cu_frame_locals_offset(struct compilation_unit *cu)
{
	assert(!"not implemented");
}

