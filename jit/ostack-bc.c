/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode operand stack
 * management instructions to immediate representation of the JIT compiler.
 */

#include <jit/bytecode-converters.h>
#include <jit/jit-compiler.h>
#include <jit/statement.h>

#include <vm/bytecodes.h>
#include <vm/stack.h>

#include <errno.h>

int convert_pop(struct parse_context *ctx)
{
	struct expression *expr = stack_pop(ctx->cu->expr_stack);
	
	if (is_invoke_expr(expr)) {
		struct statement *expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			return -ENOMEM;
			
		expr_stmt->expression = &expr->node;
		bb_add_stmt(ctx->bb, expr_stmt);
	}
	return 0;
}

int convert_dup(struct parse_context *ctx)
{
	void *v;

	v = stack_pop(ctx->cu->expr_stack);
	stack_push(ctx->cu->expr_stack, v);
	stack_push(ctx->cu->expr_stack, v);

	return 0;
}

int convert_dup_x1(struct parse_context *ctx)
{
	void *v1, *v2;

	v1 = stack_pop(ctx->cu->expr_stack);
	v2 = stack_pop(ctx->cu->expr_stack);

	stack_push(ctx->cu->expr_stack, v1);
	stack_push(ctx->cu->expr_stack, v2);
	stack_push(ctx->cu->expr_stack, v1);

	return 0;
}

int convert_dup_x2(struct parse_context *ctx)
{
	void *v1, *v2, *v3;

	v1 = stack_pop(ctx->cu->expr_stack);
	v2 = stack_pop(ctx->cu->expr_stack);
	v3 = stack_pop(ctx->cu->expr_stack);

	stack_push(ctx->cu->expr_stack, v1);
	stack_push(ctx->cu->expr_stack, v3);
	stack_push(ctx->cu->expr_stack, v2);
	stack_push(ctx->cu->expr_stack, v1);

	return 0;
}

int convert_swap(struct parse_context *ctx)
{
	void *v1, *v2;

	v1 = stack_pop(ctx->cu->expr_stack);
	v2 = stack_pop(ctx->cu->expr_stack);

	stack_push(ctx->cu->expr_stack, v1);
	stack_push(ctx->cu->expr_stack, v2);

	return 0;
}
