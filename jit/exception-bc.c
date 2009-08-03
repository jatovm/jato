/*
 * Copyright (c) 2008 Saeed Siam
 * Copyright (c) 2009 Tomasz Grabiec
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

#include "jit/bytecode-to-ir.h"
#include "jit/expression.h"
#include "jit/statement.h"
#include "jit/compiler.h"

#include "vm/stack.h"
#include "vm/die.h"

#include <errno.h>

int convert_athrow(struct parse_context *ctx)
{
	struct stack *mimic_stack = ctx->bb->mimic_stack;
	struct expression *exception_ref;
	struct expression *nullcheck;
	struct statement *stmt;

	stmt = alloc_statement(STMT_ATHROW);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	exception_ref = stack_pop(mimic_stack);

	nullcheck = null_check_expr(exception_ref);
	if (!nullcheck)
		return warn("out of memory"), -ENOMEM;

	stmt->exception_ref = &nullcheck->node;

	/*
	 * According to the JVM specification athrow operation is
	 * supposed to discard the java stack and push exception
	 * reference on it. We don't do the latter because exception
	 * reference is not transferred to exception handlers in
	 * BC2IR layer.
	 */
	while (!stack_is_empty(mimic_stack)) {
		struct expression *expr = stack_pop(mimic_stack);

		expr_put(expr);
	}

	convert_statement(ctx, stmt);

	return 0;
}
