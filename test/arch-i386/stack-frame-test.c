/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/expression.h>
#include <jit/x86-frame.h>
#include <vm/vm.h>

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
	assert_local_offset( 8, local_expr(J_INT, 0), 2);
	assert_local_offset(12, local_expr(J_INT, 1), 2);
}

void test_should_map_local_variables_after_last_arg_at_negative_offsets(void)
{
	assert_local_offset( 8, local_expr(J_INT, 0), 1);
	assert_local_offset(-4, local_expr(J_INT, 1), 1);
	assert_local_offset(-8, local_expr(J_INT, 2), 1);
}
