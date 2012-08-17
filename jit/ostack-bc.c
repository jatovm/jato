/*
 * Copyright (C) 2005-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode operand stack
 * management instructions to immediate representation of the JIT compiler.
 */

#include "jit/bytecode-to-ir.h"

#include "jit/statement.h"
#include "jit/compiler.h"

#include "vm/bytecode.h"
#include "vm/die.h"

#include "lib/stack.h"

#include <errno.h>

int convert_pop(struct parse_context *ctx)
{
	expr_put(stack_pop(ctx->bb->mimic_stack));

	return 0;
}

int convert_pop2(struct parse_context *ctx)
{
	struct expression *another_expr;
	struct expression *expr;

	expr = stack_pop(ctx->bb->mimic_stack);

	if (vm_type_is_pair(expr->vm_type))
		goto out;

	another_expr = stack_peek(ctx->bb->mimic_stack);

	if (vm_type_is_pair(another_expr->vm_type))
		goto out;

	expr_put(stack_pop(ctx->bb->mimic_stack));
out:
	expr_put(expr);

	return 0;
}

struct expression *
dup_expr(struct parse_context *ctx, struct expression *expr)
{
	struct expression *dest;
	struct statement *stmt;

	dest = temporary_expr(expr->vm_type, ctx->cu);

	expr_get(dest);
	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &dest->node;
	stmt->store_src  = &expr->node;
	convert_statement(ctx, stmt);

	return dest;
}

/**
 * Returns a pure expression for given expression. If @expr is not
 * pure we need to save it's value to a temporary and return the
 * temporary.
 */
struct expression *
get_pure_expr(struct parse_context *ctx, struct expression *expr)
{
	if (expr_is_pure(expr))
		return expr;

	return dup_expr(ctx, expr);
}

static int __convert_dup(struct parse_context *ctx, struct expression *value)
{
	struct expression *dup;

	dup = get_pure_expr(ctx, value);

	convert_expression(ctx, dup);
	convert_expression(ctx, dup_expr(ctx, expr_get(dup)));

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

	dup = get_pure_expr(ctx, value1);

	convert_expression(ctx, dup);
	convert_expression(ctx, value2);
	convert_expression(ctx, dup_expr(ctx, expr_get(dup)));

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

	dup = get_pure_expr(ctx, value1);

	convert_expression(ctx, dup);
	convert_expression(ctx, value3);
	convert_expression(ctx, value2);
	convert_expression(ctx, dup_expr(ctx, expr_get(dup)));

	return 0;
}

int convert_dup_x2(struct parse_context *ctx)
{
	struct expression *value1, *value2, *value3;

	value1 = stack_pop(ctx->bb->mimic_stack);
	value2 = stack_pop(ctx->bb->mimic_stack);

	if (vm_type_slot_size(value2->vm_type) == 2)
		return __convert_dup_x1(ctx, value1, value2);

	value3 = stack_pop(ctx->bb->mimic_stack);

	return __convert_dup_x2(ctx, value1, value2, value3);
}

static int __convert_dup2(struct parse_context *ctx, struct expression *value1, struct expression *value2)
{
	struct expression *dup, *dup2;

	dup = get_pure_expr(ctx, value1);
	dup2 = get_pure_expr(ctx, value2);

	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);
	convert_expression(ctx, dup_expr(ctx, expr_get(dup2)));
	convert_expression(ctx, dup_expr(ctx, expr_get(dup)));

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

	dup = get_pure_expr(ctx, value1);
	dup2 = get_pure_expr(ctx, value2);

	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);
	convert_expression(ctx, value3);
	convert_expression(ctx, dup_expr(ctx, expr_get(dup2)));
	convert_expression(ctx, dup_expr(ctx, expr_get(dup)));

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

	dup = get_pure_expr(ctx, value1);
	dup2 = get_pure_expr(ctx, value2);

	convert_expression(ctx, dup2);
	convert_expression(ctx, dup);
	convert_expression(ctx, value4);
	convert_expression(ctx, value3);
	convert_expression(ctx, dup_expr(ctx, expr_get(dup2)));
	convert_expression(ctx, dup_expr(ctx, expr_get(dup)));

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
