#include "arch/stack-frame.h"

#include "jit/expression.h"
#include "vm/method.h"

#include <assert.h>

/* The frame layout used for ARM is similar to GCC
 *		|  ARG n  |
 *		|    :    |
 *		|    :    |
 *		|  ARG 5  |
 *		| OLD FP  |  -> Current Frame pointer is pointing here
 *		| OLD LR  |
 *		|   R4    |
 *		|    :    |
 *		|    :    |
 *		|  R10    |
 *		| LOCAL n |
 *		|    :    |
 *		|    :    |
 *		| LOCAL 1 |
 *		|  ARG 1  |
 *		|    :    |
 *		|    :    |
 *		|  ARG 4  |
 */

unsigned long arg_offset(struct stack_frame *frame, unsigned long idx)
{
	unsigned long nr_args = frame->nr_args;


	if (nr_args <= 4)
		return 0UL - (9 + frame->nr_local_slots - nr_args + idx) * sizeof(unsigned long);
	else {
		if (idx < 4)
			return 0UL - (9 + frame->nr_local_slots - nr_args + idx) * sizeof(unsigned long);
		else
			return (idx - 3) * sizeof(unsigned long);
	}
}

unsigned long local_offset(struct stack_frame *frame, unsigned long idx)
{
	return 0UL - (8 + frame->nr_local_slots - frame->nr_args - idx) * sizeof(unsigned long);
}

unsigned long frame_local_offset(struct vm_method *vm_method, struct expression *expression)
{
	assert(!"not implemented");
}

unsigned long slot_offset(struct stack_slot *slot)
{
	struct stack_frame *frame = slot->parent;
	unsigned long index = slot->index;
	if (index < frame->nr_args)
		return arg_offset(frame, index);
	else
		return local_offset(frame, index);
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

