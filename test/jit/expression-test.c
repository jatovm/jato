/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/expression.h>

#include <libharness.h>
#include <stdlib.h>

static void assert_nr_args(unsigned long expected, struct expression *args_list)
{
	assert_int_equals(expected, nr_args(args_list));
	expr_put(args_list);
}

void test_should_count_zero_arguments_for_noargs(void)
{
	assert_nr_args(0, no_args_expr());
}

void test_should_count_all_arguments_in_list(void)
{
	assert_nr_args(1, arg_expr(value_expr(J_INT, 0)));
	assert_nr_args(2, args_list_expr(arg_expr(value_expr(J_INT, 0)), arg_expr(value_expr(J_INT, 1))));
}
