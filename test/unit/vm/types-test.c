/*
 * Copyright (c) 2008  Arthur Huillet
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
