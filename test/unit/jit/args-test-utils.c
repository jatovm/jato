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

#include "jit/compilation-unit.h"
#include "jit/expression.h"

#include <bc-test-utils.h>
#include <libharness.h>

void create_args(struct expression **args, int nr_args)
{
        int i;

        for (i = 0; i < nr_args; i++) {
                args[i] = value_expr(J_INT, i);
        }
}

void push_args(struct basic_block *bb, struct expression **args, int nr_args)
{
        int i;

        for (i = 0; i < nr_args; i++) {
                stack_push(bb->mimic_stack, args[i]);
        }
}

void assert_args(struct expression **expected_args,int nr_args,
		 struct expression *args_list)
{
	int i;
	struct expression *tree = args_list;
	struct expression *actual_args[nr_args];

	if (nr_args == 0) {
		assert_int_equals(EXPR_NO_ARGS, expr_type(args_list));
		return;
	}

	i = 0;
	while (i < nr_args) {
		if (expr_type(tree) == EXPR_ARGS_LIST) {
			struct expression *expr = to_expr(tree->node.kids[1]);
			actual_args[i++] = to_expr(expr->arg_expression);
			tree = to_expr(tree->node.kids[0]);
		} else if (expr_type(tree) == EXPR_ARG) {
			actual_args[i++] = to_expr(tree->arg_expression);
			break;
		} else {
			assert_true(false);
			break;
		}
	}

	assert_int_equals(i, nr_args);

	for (i = 0; i < nr_args; i++)
		assert_ptr_equals(expected_args[i], actual_args[i]);
}
