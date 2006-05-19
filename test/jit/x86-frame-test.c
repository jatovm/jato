/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/expression.h>
#include <jit/x86-frame.h>

#include <libharness.h>

static void assert_local_offset(unsigned long expected, struct expression *local)
{
	assert_int_equals(expected, frame_local_offset(local));
	expr_put(local);
}

void test_should_map_local_index_to_frame_offset(void)
{
	assert_local_offset( 8, local_expr(J_INT, 0));
	assert_local_offset(12, local_expr(J_INT, 1));
}
