/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <expression.h>
#include <stdlib.h>

struct expression *alloc_expression(enum expression_type type,
				    enum jvm_type jvm_type)
{
	struct expression *expr = malloc(sizeof(*expr));
	if (expr) {
		expr->type = type;
		expr->jvm_type = jvm_type;
	}
	return expr;
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

struct expression *temporary_expr(enum jvm_type jvm_type, unsigned long temporary)
{
	struct expression *expr = alloc_expression(EXPR_TEMPORARY, jvm_type);
	if (expr)
		expr->temporary = temporary;

	return expr;
}

struct expression *array_deref_expr(enum jvm_type jvm_type, unsigned long arrayref,
				    unsigned long array_index)
{
	struct expression *expr = alloc_expression(EXPR_ARRAY_DEREF, jvm_type);
	if (expr) {
		expr->arrayref = arrayref;
		expr->array_index = array_index;
	}
	return expr;
}

void free_expression(struct expression *expr)
{
	free(expr);
}
