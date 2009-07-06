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

#include "jit/expression.h"
#include "jit/args.h"

#include "vm/stack.h"

struct expression *
insert_arg(struct expression *root, struct expression *expr)
{
	struct expression *_expr;

	_expr = arg_expr(expr);
	_expr->bytecode_offset = expr->bytecode_offset;

	if (!root)
		return _expr;

	return args_list_expr(root, _expr);
}

struct expression *
convert_args(struct stack *mimic_stack, unsigned long nr_args)
{
	struct expression *args_list = NULL;
	unsigned long i;

	if (nr_args == 0) {
		args_list = no_args_expr();
		goto out;
	}

	for (i = 0; i < nr_args; i++) {
		struct expression *expr = stack_pop(mimic_stack);
		args_list = insert_arg(args_list, expr);
	}

  out:
	return args_list;
}

const char *parse_method_args(const char *type_str, enum vm_type *vmtype)
{
	char type_name[] = { "X" };

	if (*type_str == '(')
		type_str++;

	if (*type_str == ')')
		return NULL;

	if (*type_str == '[') {
		*vmtype = J_REFERENCE;
		type_str++;

		if (*type_str != 'L') {
			return type_str + 1;
		}
	}

	if (*type_str == 'L') {
		++type_str;
		while (*(type_str++) != ';')
			;
		*vmtype = J_REFERENCE;
	} else {
		type_name[0] = *(type_str++);
		*vmtype = str_to_type(type_name);
	}

	return type_str;
}
