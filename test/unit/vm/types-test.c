/*
 * Copyright (c) 2008  Arthur Huillet
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/types.h"

#include <libharness.h>

void test_str_to_type(void)
{
	assert_int_equals(J_VOID, str_to_type("V"));
	assert_int_equals(J_BYTE, str_to_type("B"));
	assert_int_equals(J_CHAR, str_to_type("C"));
	assert_int_equals(J_DOUBLE, str_to_type("D"));
	assert_int_equals(J_FLOAT, str_to_type("F"));
	assert_int_equals(J_INT, str_to_type("I"));
	assert_int_equals(J_LONG, str_to_type("J"));
	assert_int_equals(J_SHORT, str_to_type("S"));
	assert_int_equals(J_BOOLEAN, str_to_type("Z"));
	assert_int_equals(J_REFERENCE, str_to_type("[L"));
	assert_int_equals(J_REFERENCE, str_to_type("[java/lang/Object;"));
}

void test_get_method_return_type(void)
{
	assert_int_equals(J_VOID, get_method_return_type("(BCDFIJSZ)V"));
	assert_int_equals(J_BYTE, get_method_return_type("(DD)B"));
	assert_int_equals(J_LONG, get_method_return_type("(V)J"));
}
