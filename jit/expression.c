/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <expression.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

struct expression *alloc_expression(enum expression_type type,
				    enum jvm_type jvm_type)
{
	struct expression *expr = malloc(sizeof *expr);
	if (expr) {
		memset(expr, 0, sizeof *expr);
		expr->node.op = type << EXPR_TYPE_SHIFT;
		expr->jvm_type = jvm_type;
		expr->refcount = 1;
	}
	return expr;
}

void free_expression(struct expression *expr)
{
	if (!expr)
		return;

	switch (expr_type(expr)) {
	case EXPR_VALUE:
	case EXPR_FVALUE:
	case EXPR_LOCAL:
	case EXPR_TEMPORARY:
		/* nothing to do */
		break;
	case EXPR_ARRAY_DEREF:
		if (expr->arrayref)
			expr_put(to_expr(expr->arrayref));
		if (expr->array_index)
			expr_put(to_expr(expr->array_index));
		break;
	case EXPR_BINOP:
		if (expr->binary_left)
			expr_put(to_expr(expr->binary_left));
		if (expr->binary_right)
			expr_put(to_expr(expr->binary_right));
		break;
	case EXPR_UNARY_OP:
		if (expr->unary_expression)
			expr_put(to_expr(expr->unary_expression));
		break;
	case EXPR_CONVERSION:
		if (expr->from_expression)
			expr_put(to_expr(expr->from_expression));
		break;
	case EXPR_FIELD:
		/* nothing to do */
		break;
	case EXPR_INVOKE:
		if (expr->args_list)
			expr_put(to_expr(expr->args_list));
		break;
	case EXPR_ARGS_LIST:
		expr_put(to_expr(expr->args_left));
		expr_put(to_expr(expr->args_right));
		break;
	case EXPR_ARG:
		expr_put(to_expr(expr->arg_expression));
		break;
	};
	free(expr);
}

void expr_get(struct expression *expr)
{
	expr->refcount++;
}

void expr_put(struct expression *expr)
{
	expr->refcount--;
	if (expr->refcount == 0)
		free_expression(expr);
}

struct expression *value_expr(enum jvm_type jvm_type, unsigned long long value)
{
	struct expression *expr = alloc_expression(EXPR_VALUE, jvm_type);
	if (expr)
		expr->value = value;

	return expr;
}

struct expression *fvalue_expr(enum jvm_type jvm_type, double fvalue)
{
	struct expression *expr = alloc_expression(EXPR_FVALUE, jvm_type);
	if (expr)
		expr->fvalue = fvalue;

	return expr;
}

struct expression *local_expr(enum jvm_type jvm_type, unsigned long local_index)
{
	struct expression *expr = alloc_expression(EXPR_LOCAL, jvm_type);
	if (expr)
		expr->local_index = local_index;

	return expr;
}

struct expression *temporary_expr(enum jvm_type jvm_type,
				  unsigned long temporary)
{
	struct expression *expr = alloc_expression(EXPR_TEMPORARY, jvm_type);
	if (expr)
		expr->temporary = temporary;

	return expr;
}

struct expression *array_deref_expr(enum jvm_type jvm_type,
				    struct expression *arrayref,
				    struct expression *array_index)
{
	struct expression *expr = alloc_expression(EXPR_ARRAY_DEREF, jvm_type);
	if (expr) {
		expr->arrayref = &arrayref->node;
		expr->array_index = &array_index->node;
	}
	return expr;
}

struct expression *binop_expr(enum jvm_type jvm_type,
			      enum binary_operator binary_operator,
			      struct expression *binary_left, struct expression *binary_right)
{
	struct expression *expr = alloc_expression(EXPR_BINOP, jvm_type);
	if (expr) {
		expr->node.op |= binary_operator << BIN_OP_SHIFT;
		expr->binary_left = &binary_left->node;
		expr->binary_right = &binary_right->node;
	}
	return expr;
}

struct expression *unary_op_expr(enum jvm_type jvm_type,
				 enum unary_operator unary_operator,
				 struct expression *unary_expression)
{
	struct expression *expr = alloc_expression(EXPR_UNARY_OP, jvm_type);
	if (expr) {
		expr->node.op |= unary_operator << UNARY_OP_SHIFT;
		expr->unary_expression = &unary_expression->node;
	}
	return expr;
}

struct expression *conversion_expr(enum jvm_type jvm_type,
				   struct expression *from_expression)
{
	struct expression *expr = alloc_expression(EXPR_CONVERSION, jvm_type);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *field_expr(enum jvm_type jvm_type,
			      struct fieldblock *field)
{
	struct expression *expr = alloc_expression(EXPR_FIELD, jvm_type);
	if (expr)
		expr->field = field;
	return expr;
}

struct expression *invoke_expr(enum jvm_type jvm_type,
			     struct methodblock *target_method)
{
	struct expression *expr = alloc_expression(EXPR_INVOKE, jvm_type);
	if (expr) {
		expr->target_method = target_method;
	}
	return expr;
}

struct expression *args_list_expr(struct expression *args_left,
				  struct expression *args_right)
{
	struct expression *expr = alloc_expression(EXPR_ARGS_LIST, J_VOID);
	if (expr) {
		expr->args_left = &args_left->node;
		expr->args_right = &args_right->node;
	}
	return expr;
}

struct expression *arg_expr(struct expression *arg_expression)
{
	struct expression *expr = alloc_expression(EXPR_ARG, J_VOID);
	if (expr)
		expr->arg_expression = &arg_expression->node;
	return expr;
}
