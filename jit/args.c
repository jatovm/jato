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

#include <assert.h>

#include "jit/expression.h"
#include "jit/args.h"

#include "vm/method.h"
#include "lib/stack.h"

#ifdef CONFIG_ARGS_MAP
static inline void set_expr_arg_reg(struct expression *expr,
				    struct vm_method *method,
				    int index)
{
	expr->arg_reg = method->args_map[index].reg;
}
#else /* CONFIG_ARGS_MAP */
static void set_expr_arg_reg(struct expression *expr,
			     struct vm_method *method,
			     int index)
{
}
#endif /* CONFIG_ARGS_MAP */

static bool is_this_arg(struct vm_method *method, int index)
{
	if (vm_method_is_static(method))
		return false;

	if (vm_method_is_jni(method))
		return index == 1;

	return index == 0;
}

static struct expression *
insert_arg(struct expression *root, struct expression *expr, struct vm_method *method, int index)
{
	struct expression *_expr;

	/* Check if we should put @expr in EXPR_ARG_THIS. */
	if (is_this_arg(method, index))
		_expr = arg_this_expr(expr);
	else
		_expr = arg_expr(expr);

	_expr->bytecode_offset = expr->bytecode_offset;
	set_expr_arg_reg(_expr, method, index);

	if (!root)
		return _expr;

	return args_list_expr(root, _expr);
}

struct expression *
convert_args(struct stack *mimic_stack, unsigned long nr_args, struct vm_method *method)
{
	struct expression *args_list = NULL;
	unsigned long nr_total_args;
	unsigned long i;

	nr_total_args = nr_args;

	if (vm_method_is_jni(method)) {
		if (vm_method_is_static(method))
			nr_total_args++;

		nr_total_args++;
	}

	if (nr_total_args == 0) {
		args_list = no_args_expr();
		goto out;
	}

	/*
	 * We scan the args map in reverse order, since the order of arguments
	 * is already reversed.
	 */
	for (i = 0; i < nr_args; i++) {
		struct expression *expr = stack_pop(mimic_stack);

		if (vm_type_is_pair(expr->vm_type))
			i++;

		if (i >= nr_args)
			break;

		args_list = insert_arg(args_list, expr, method, nr_total_args - i - 1);
	}

	if (vm_method_is_jni(method)) {
		struct expression *expr;

		if (vm_method_is_static(method)) {
			expr = value_expr(J_REFERENCE, (unsigned long) method->class->object);
			if (!expr)
				goto error;

			args_list = insert_arg(args_list, expr, method, 1);
		}

		expr = value_expr(J_REFERENCE, (unsigned long) vm_jni_get_jni_env());
		if (!expr)
			goto error;

		args_list = insert_arg(args_list, expr, method, 0);
	}
  out:
	return args_list;
  error:
	expr_put(args_list);
	return NULL;
}

static struct expression *
insert_native_arg(struct expression *root, struct expression *expr)
{
	struct expression *_expr;

	_expr = arg_expr(expr);
	_expr->bytecode_offset = expr->bytecode_offset;

	if (!root)
		return _expr;

	return args_list_expr(root, _expr);
}

/**
 * This function prepares argument list that will be used
 * with the native VM call. All arguments are passed on stack.
 */
struct expression *
convert_native_args(struct stack *mimic_stack, unsigned long nr_args)
{
	struct expression *args_list = NULL;
	unsigned long i;

	if (nr_args == 0) {
		args_list = no_args_expr();
		goto out;
	}

	for (i = 0; i < nr_args; i++) {
		struct expression *expr = stack_pop(mimic_stack);
		args_list = insert_native_arg(args_list, expr);
	}

  out:
	return args_list;
}
