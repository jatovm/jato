/*
 * Copyright (C) 2006-2008  Pekka Enberg
 */

#include <jit/expression.h>
#include <vm/vm.h>
#include <arch/stack-frame.h>
#include <stdlib.h>
#include <libharness.h>

static void assert_local_offset(long expected, struct expression *local, unsigned long nr_args)
{
	struct methodblock method = {
		.args_count = nr_args
	};
	assert_int_equals(expected, frame_local_offset(&method, local));
	expr_put(local);
}

void test_should_map_local_index_to_frame_offset(void)
{
	assert_local_offset(20, local_expr(J_INT, 0), 2);
	assert_local_offset(24, local_expr(J_INT, 1), 2);
}

void test_should_map_local_variables_after_last_arg_at_negative_offsets(void)
{
	assert_local_offset(20, local_expr(J_INT, 0), 1);
	assert_local_offset(-4, local_expr(J_INT, 1), 1);
	assert_local_offset(-8, local_expr(J_INT, 2), 1);
}

#define NR_ARGS 2
#define NR_LOCAL_SLOTS 4

void test_arguments_are_at_successive_positive_offsets(void)
{
	struct stack_slot *param1, *param2;
	struct stack_frame *frame;

	frame = alloc_stack_frame(NR_ARGS, NR_LOCAL_SLOTS);

	param1 = get_local_slot(frame, 0);
	param2 = get_local_slot(frame, 1);

	assert_int_equals(0x14, slot_offset(param1));
	assert_int_equals(0x18, slot_offset(param2));

	free_stack_frame(frame);
}

void test_locals_are_at_successive_negative_offsets(void)
{
	struct stack_slot *local1, *local2;
	struct stack_frame *frame;

	frame = alloc_stack_frame(NR_ARGS, NR_LOCAL_SLOTS);

	local1 = get_local_slot(frame, 2);
	local2 = get_local_slot(frame, 3);

	assert_int_equals(0UL - 0x4, slot_offset(local1));
	assert_int_equals(0UL - 0x8, slot_offset(local2));
	assert_int_equals(8, frame_locals_size(frame));

	free_stack_frame(frame);
}

void test_spill_storage_is_at_successive_negative_offsets_after_locals(void)
{
	struct stack_slot *spill1, *spill2;
	struct stack_frame *frame;

	frame = alloc_stack_frame(NR_ARGS, NR_LOCAL_SLOTS);

	spill1 = get_spill_slot_64(frame);
	spill2 = get_spill_slot_32(frame);

	assert_int_equals(0UL - 0xc, slot_offset(spill1));
	assert_int_equals(0UL - 0x14, slot_offset(spill2));
	assert_int_equals(20, frame_locals_size(frame));

	free_stack_frame(frame);
}
