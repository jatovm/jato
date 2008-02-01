/*
 * Copyright (C) 2005-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode operand stack
 * management instructions to immediate representation of the JIT compiler.
 */

#include <jit/bytecode-converters.h>
#include <jit/compiler.h>
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

static struct expression *dup_expr(struct parse_context *ctx, struct expression *expr)
{
	struct expression *dest;
	struct statement *stmt;
	struct var_info *var;

	var = get_var(ctx->cu);
	dest = var_expr(J_REFERENCE, var);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &dest->node;
	stmt->store_src  = &expr->node;
	bb_add_stmt(ctx->bb, stmt);

	return dest;
}

int convert_dup(struct parse_context *ctx)
{
	struct expression *dup, *value;

	value = stack_pop(ctx->cu->expr_stack);
	dup = dup_expr(ctx, value);

	stack_push(ctx->cu->expr_stack, dup);
	stack_push(ctx->cu->expr_stack, dup);

	return 0;
}

int convert_dup_x1(struct parse_context *ctx)
{
	struct expression *value1, *value2, *dup;

	value1 = stack_pop(ctx->cu->expr_stack);
	value2 = stack_pop(ctx->cu->expr_stack);
	dup = dup_expr(ctx, value1);

	stack_push(ctx->cu->expr_stack, dup);
	stack_push(ctx->cu->expr_stack, value2);
	stack_push(ctx->cu->expr_stack, dup);

	return 0;
}

int convert_dup_x2(struct parse_context *ctx)
{
	struct expression *value1, *value2, *value3, *dup;

	value1 = stack_pop(ctx->cu->expr_stack);
	value2 = stack_pop(ctx->cu->expr_stack);
	value3 = stack_pop(ctx->cu->expr_stack);
	dup = dup_expr(ctx, value1);

	stack_push(ctx->cu->expr_stack, dup);
	stack_push(ctx->cu->expr_stack, value3);
	stack_push(ctx->cu->expr_stack, value2);
	stack_push(ctx->cu->expr_stack, dup);

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
