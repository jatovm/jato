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
	struct expression *expr = stack_pop(ctx->bb->mimic_stack);
	
	if (is_invoke_expr(expr)) {
		struct statement *expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			return -ENOMEM;
			
		expr_stmt->expression = &expr->node;
		convert_statement(ctx, expr_stmt);
	}
	return 0;
}

static struct expression *dup_expr(struct parse_context *ctx, struct expression *expr)
{
	struct expression *dest;
	struct statement *stmt;
	struct var_info *tmp_low, *tmp_high = NULL;

	tmp_low = get_var(ctx->cu);

	if (expr->vm_type == J_LONG) {
		tmp_high = get_var(ctx->cu);
	}

	dest = temporary_expr(expr->vm_type, tmp_high, tmp_low);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &dest->node;
	stmt->store_src  = &expr->node;
	convert_statement(ctx, stmt);

	return dest;
}

static int __convert_dup(struct parse_context *ctx, struct expression *value)
{
	struct expression *dup;

	dup = dup_expr(ctx, value);

	convert_expression(ctx, dup);
	convert_expression(ctx, dup);
	return 0;
}

int convert_dup(struct parse_context *ctx)
{
	struct expression *value;

	value = stack_pop(ctx->bb->mimic_stack);

	return __convert_dup(ctx, value);
}

static int __convert_dup_x1(struct parse_context *ctx, struct expression *value1, struct expression *value2)
{
	struct expression *dup;

	dup = dup_expr(ctx, value1);

	convert_expression(ctx, dup);
	convert_expression(ctx, value2);
	convert_expression(ctx, dup);
	return 0;
}

int convert_dup_x1(struct parse_context *ctx)
{
	struct expression *value1, *value2;

	value1 = stack_pop(ctx->bb->mimic_stack);
	value2 = stack_pop(ctx->bb->mimic_stack);

	return __convert_dup_x1(ctx, value1, value2);
}

static int __convert_dup_x2(struct parse_context *ctx, struct expression *value1, struct expression *value2, struct expression *value3)
{
	struct expression *dup;

	dup = dup_expr(ctx, value1);

	convert_expression(ctx, dup);
	convert_expression(ctx, value3);
	convert_expression(ctx, value2);
	convert_expression(ctx, dup);

	return 0;
}

int convert_dup_x2(struct parse_context *ctx)
{
	struct expression *value1, *value2, *value3;

	value1 = stack_pop(ctx->bb->mimic_stack);
	value2 = stack_pop(ctx->bb->mimic_stack);
	value3 = stack_pop(ctx->bb->mimic_stack);

	return __convert_dup_x2(ctx, value1, value2, value3);
}

static int __convert_dup2(struct parse_context *ctx, struct expression *value1, struct expression *value2)
{
	struct expression *dup, *dup2;
	dup = dup_expr(ctx, value1);
	dup2 = dup_expr(ctx, value2);

	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);
	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);

	return 0;
}

int convert_dup2(struct parse_context *ctx)
{
	struct expression *value, *value2;
	/* Dup2 duplicates *one* 64bit operand or *two* 32 bit operands.
	 * This second case is to be handled here. */
	value = stack_pop(ctx->bb->mimic_stack);

	if (value->vm_type == J_LONG || value->vm_type == J_DOUBLE) {
		return __convert_dup(ctx, value);
	} else {
		value2 = stack_pop(ctx->bb->mimic_stack);
		return __convert_dup2(ctx, value, value2);
	}
}

static int __convert_dup2_x1(struct parse_context * ctx, struct expression * value1, struct expression *value2, struct expression *value3)
{
	struct expression *dup, *dup2;

	dup = dup_expr(ctx, value1);
	dup2 = dup_expr(ctx, value2);

	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);
	convert_expression(ctx, value3);
	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);

	return 0;
}

int convert_dup2_x1(struct parse_context *ctx)
{
	struct expression *value1, *value2, *value3;
	/* Dup2 duplicates *one* 64bit operand or *two* 32 bit operands.
	 * This second case is to be handled here. */
	value1 = stack_pop(ctx->bb->mimic_stack);
	value2 = stack_pop(ctx->bb->mimic_stack);

	if (value1->vm_type == J_LONG || value1->vm_type == J_DOUBLE) {
		return __convert_dup_x1(ctx, value1, value2);
	} else {
		value3 = stack_pop(ctx->bb->mimic_stack);
		return __convert_dup2_x1(ctx, value1, value2, value3);
	}
}

static int __convert_dup2_x2(struct parse_context *ctx, struct expression *value1, struct expression *value2, struct expression *value3, struct expression *value4)
{
	struct expression *dup, *dup2;
	dup = dup_expr(ctx, value1);
	dup2 = dup_expr(ctx, value2);

	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);
	convert_expression(ctx, value4);
	convert_expression(ctx, value3);
	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);

	return 0;
}

int convert_dup2_x2(struct parse_context *ctx)
{
	struct expression *value1, *value2, *value3, *value4;

	/* Refer to the JVM spec at http://java.sun.com/docs/books/jvms/second_edition/html/Instructions2.doc3.html#dup2_x2
	 */
	value1 = stack_pop(ctx->bb->mimic_stack);
	value2 = stack_pop(ctx->bb->mimic_stack);

	if (value1->vm_type == J_LONG || value1->vm_type == J_DOUBLE) {
		if (value2->vm_type == J_LONG || value2->vm_type == J_DOUBLE) {
			/* Call dup_x1 - form 4*/
			return __convert_dup_x1(ctx, value1, value2);
		} else {
			/* Call dup_x2 - form 2*/
			value3 = stack_pop(ctx->bb->mimic_stack);
			return __convert_dup_x2(ctx, value1, value2, value3);
		}
	} else {
		value3 = stack_pop(ctx->bb->mimic_stack);

		if (value3->vm_type == J_LONG || value3->vm_type == J_DOUBLE) {
			/* Call dup2_x1 - form 3*/
			return __convert_dup2_x1(ctx, value1, value2, value3);
		} else {
			/* the annoying case - form 1 */
			value4 = stack_pop(ctx->bb->mimic_stack);
			return __convert_dup2_x2(ctx, value1, value2, value3, value4);
		}
	}
	return 0;
}

int convert_swap(struct parse_context *ctx)
{
	void *v1, *v2;

	v1 = stack_pop(ctx->bb->mimic_stack);
	v2 = stack_pop(ctx->bb->mimic_stack);

	convert_expression(ctx, v1);
	convert_expression(ctx, v2);

	return 0;
}
