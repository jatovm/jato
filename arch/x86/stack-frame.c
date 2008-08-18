/*
 * Copyright (C) 2006-2008  Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <jit/expression.h>
#include <vm/vm.h>
#include <arch/stack-frame.h>
#include <stdlib.h>

#define ARGS_START_OFFSET (sizeof(unsigned long) * 5)

static unsigned long index_to_offset(unsigned long index)
{
	return index * sizeof(unsigned int);
}

long frame_local_offset(struct methodblock *method, struct expression *local)
{
	unsigned long idx, nr_args;

	idx = local->local_index;
	nr_args = method->args_count;

	if (idx < nr_args)
		return ARGS_START_OFFSET + index_to_offset(idx);

	return 0 - index_to_offset(idx - nr_args + 1);
}

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
