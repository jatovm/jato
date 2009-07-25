/*
 * Copyright (C) 2009 Eduard - Gabriel Munteanu <eduard.munteanu@linux360.ro>
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
#include <stdio.h>

#include "arch/args.h"

#include "jit/args.h"
#include "jit/expression.h"

#include "vm/method.h"
#include "vm/types.h"

#ifdef CONFIG_X86_64

int args_init(unsigned long *state,
	      struct vm_method *method,
	      unsigned long nr_args)
{
	const char *type;
	enum vm_type vm_type;

	for (type = method->type + 1; *type != ')'; skip_type(&type)) {
		vm_type = str_to_type(type);
		switch (vm_type) {
		case J_BYTE:
		case J_CHAR:
		case J_INT:
		case J_LONG:
		case J_SHORT:
		case J_BOOLEAN:
		case J_REFERENCE:
			method->reg_args_count++;
			break;
		default:
			NOT_IMPLEMENTED;
			break;
		}

		if (method->reg_args_count == 6)
			break;
	}

	if (vm_method_is_jni(method))
		method->reg_args_count += 2;

	method->reg_args_count = min(method->reg_args_count, 6);
	method->args_count -= method->reg_args_count;

	*state = 1;

	return 0;
}

int args_set(unsigned long *state,
	     struct vm_method *method,
	     struct expression *expr)
{
	struct expression *value_expr = to_expr(expr->arg_expression);
	unsigned long reg;

	if ((int) *state <= method->args_count) {
		(*state)++;
		expr->arg_private = NULL;
		return 0;
	}

	assert((value_expr->vm_type == J_INT ||
		value_expr->vm_type == J_LONG ||
		value_expr->vm_type == J_REFERENCE));

	reg = (6 - method->reg_args_count) + (*state)++ - method->args_count;
	expr->arg_private = (void *) reg;

	return 0;
}

static enum machine_reg __args_select_reg(unsigned long arg_idx)
{
	switch (arg_idx) {
	case 0:
		return REG_UNASSIGNED;
	case 6:
		return REG_RDI;
	case 5:
		return REG_RSI;
	case 4:
		return REG_RDX;
	case 3:
		return REG_RCX;
	case 2:
		return REG_R8;
	case 1:
		return REG_R9;
	default:
		assert(arg_idx > 6);
		return REG_UNASSIGNED;
	}
}

enum machine_reg args_select_reg(struct expression *expr)
{
	return __args_select_reg((unsigned long) expr->arg_private);
}

enum machine_reg args_local_to_reg(struct vm_method *method, int local_idx)
{
	local_idx++;

	/* Check if local_idx refers to *this or stack parameters. */
	if (local_idx > method->reg_args_count)
		return REG_UNASSIGNED;

	return __args_select_reg(local_idx);
}

#else /* CONFIG_X86_64 */

#endif /* CONFIG_X86_64 */
