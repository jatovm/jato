/*
 * Copyright (c) 2008  Pekka Enberg
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
#include <jit/stack-slot.h>
#include <vm/stdlib.h>
#include <stdlib.h>

struct stack_frame *alloc_stack_frame(unsigned long nr_args,
				      unsigned long nr_local_slots)
{
	struct stack_frame *frame;
	unsigned long i;

	frame = zalloc(sizeof *frame);
	if (!frame)
		return NULL;

	frame->local_slots = calloc(nr_local_slots, sizeof(struct stack_slot));
	if (!frame->local_slots) {
		free(frame);
		return NULL;
	}
	for (i = 0; i < nr_local_slots; i++) {
		struct stack_slot *slot = get_local_slot(frame, i);

		slot->index  = i;
		slot->parent = frame;
	}
	frame->nr_local_slots = nr_local_slots;
	frame->nr_args = nr_args;

	return frame;
}

void free_stack_frame(struct stack_frame *frame)
{
	free(frame->local_slots);
	free(frame);
}

struct stack_slot *get_local_slot(struct stack_frame *frame, unsigned long index)
{
	return &frame->local_slots[index];
}

static struct stack_slot *
__get_spill_slot(struct stack_frame *frame, unsigned long size)
{
	struct stack_slot *slot;

	slot = &frame->spill_slots[frame->nr_spill_slots];
	slot->index = frame->nr_local_slots + frame->nr_spill_slots;
	slot->parent = frame;
	frame->nr_spill_slots += size;
	return slot;
}

struct stack_slot *get_spill_slot_32(struct stack_frame *frame)
{
	return __get_spill_slot(frame, 1);
}

struct stack_slot *get_spill_slot_64(struct stack_frame *frame)
{
	return __get_spill_slot(frame, 2);
}
