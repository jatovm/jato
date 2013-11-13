/*
 * Copyright (c) 2008 Saeed Siam
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "jit/bytecode-to-ir.h"
#include "jit/expression.h"
#include "jit/statement.h"
#include "jit/compiler.h"

#include "lib/stack.h"
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
	clear_mimic_stack(ctx->bb->mimic_stack);

	convert_statement(ctx, stmt);

	return 0;
}
