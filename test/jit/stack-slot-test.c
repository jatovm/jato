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
#include <libharness.h>
#include <stdlib.h>

#define NR_ARGS	2

/* 32-bit, 64-bit, and 32-bit slots, respectively.  */
#define NR_LOCAL_SLOTS 4

void test_local_slots_are_in_sequential_order(void)
{
	struct stack_slot *slot1, *slot2, *slot3;
	struct stack_frame *frame;

	frame = alloc_stack_frame(NR_ARGS, NR_LOCAL_SLOTS);

	slot1 = get_local_slot(frame, 0);
	assert_int_equals(0, slot1->index);

	slot2 = get_local_slot(frame, 1);
	assert_int_equals(1, slot2->index);

	slot3 = get_local_slot(frame, 3);
	assert_int_equals(3, slot3->index);

	free_stack_frame(frame);
}

void test_32_bit_spill_slot_occupies_one_stack_slot(void)
{
	struct stack_slot *slot1, *slot2;
	struct stack_frame *frame;

	frame = alloc_stack_frame(NR_ARGS, NR_LOCAL_SLOTS);

	slot1 = get_spill_slot_32(frame);
	assert_int_equals(4, slot1->index);

	slot2 = get_spill_slot_32(frame);
	assert_int_equals(5, slot2->index);

	free_stack_frame(frame);
}

void test_64_bit_spill_slot_occupies_one_stack_slot(void)
{
	struct stack_slot *slot1, *slot2;
	struct stack_frame *frame;

	frame = alloc_stack_frame(NR_ARGS, NR_LOCAL_SLOTS);

	slot1 = get_spill_slot_64(frame);
	assert_int_equals(4, slot1->index);

	slot2 = get_spill_slot_64(frame);
	assert_int_equals(6, slot2->index);

	free_stack_frame(frame);
}
