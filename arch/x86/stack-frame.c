/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for managing the native IA-32 function stack
 * frame.
 */

#include <jit/expression.h>
#include <vm/vm.h>
#include <arch/stack-frame.h>
#include <stdlib.h>

#define LOCAL_START (5 * sizeof(u4))

long frame_local_offset(struct methodblock *method, struct expression *local)
{
	unsigned long idx, nr_args;

	idx = local->local_index;
	nr_args = method->args_count;

	if (idx < nr_args)
		return (idx * sizeof(u4)) + LOCAL_START;

	return 0 - ((idx - nr_args+1) * sizeof(u4));
}

static unsigned long index_to_offset(unsigned long index)
{
	return index * sizeof(unsigned int);
}

#define ARGS_START_OFFSET (sizeof(unsigned long) * 5)

unsigned long slot_offset(struct stack_slot *slot)
{
	struct stack_frame *frame = slot->parent;
	if (slot->index < frame->nr_args)
		return ARGS_START_OFFSET + index_to_offset(slot->index);

	return 0UL - index_to_offset(slot->index - frame->nr_args + 1);
}

unsigned long frame_locals_size(struct stack_frame *frame)
{
	unsigned long nr_locals = frame->nr_local_slots - frame->nr_args;
	return index_to_offset(nr_locals + frame->nr_spill_slots);
}
