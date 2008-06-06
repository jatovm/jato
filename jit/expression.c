/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/expression.h>
#include <vm/vm.h>
#include <vm/method.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

struct expression *alloc_expression(enum expression_type type,
				    enum vm_type vm_type)
{
	struct expression *expr = malloc(sizeof *expr);
	if (expr) {
		memset(expr, 0, sizeof *expr);
		expr->node.op = type << EXPR_TYPE_SHIFT;
		expr->vm_type = vm_type;
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
	case EXPR_VAR:
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
	case EXPR_CLASS_FIELD:
		/* nothing to do */
		break;
	case EXPR_INSTANCE_FIELD:
		expr_put(to_expr(expr->objectref_expression));
		break;
	case EXPR_INVOKE:
	case EXPR_INVOKEVIRTUAL:
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
	case EXPR_NO_ARGS:
		/* nothing to do */
		break;
	case EXPR_NEW:
		/* nothing to do */
		break;
	case EXPR_NEWARRAY:
		expr_put(to_expr(expr->array_size));
		break;
	case EXPR_ANEWARRAY:
		expr_put(to_expr(expr->anewarray_size));
		break;
	case EXPR_ARRAYLENGTH:
		expr_put(to_expr(expr->arraylength_ref));
		break;
	case EXPR_LAST:
		assert(!"EXPR_LAST is not a real type. Don't use it");
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
	assert(expr->refcount > 0);

	expr->refcount--;
	if (expr->refcount == 0)
		free_expression(expr);
}

struct expression *value_expr(enum vm_type vm_type, unsigned long long value)
{
	struct expression *expr = alloc_expression(EXPR_VALUE, vm_type);
	if (expr)
		expr->value = value;

	return expr;
}

struct expression *fvalue_expr(enum vm_type vm_type, double fvalue)
{
	struct expression *expr = alloc_expression(EXPR_FVALUE, vm_type);
	if (expr)
		expr->fvalue = fvalue;

	return expr;
}

struct expression *local_expr(enum vm_type vm_type, unsigned long local_index)
{
	struct expression *expr = alloc_expression(EXPR_LOCAL, vm_type);
	if (expr)
		expr->local_index = local_index;

	return expr;
}

struct expression *temporary_expr(enum vm_type vm_type,
				  unsigned long temporary)
{
	struct expression *expr = alloc_expression(EXPR_TEMPORARY, vm_type);
	if (expr)
		expr->temporary = temporary;

	return expr;
}

struct expression *var_expr(enum vm_type vm_type, struct var_info *var)
{
	struct expression *expr = alloc_expression(EXPR_VAR, vm_type);
	if (expr)
		expr->var = var;

	return expr;
}

struct expression *array_deref_expr(enum vm_type vm_type,
				    struct expression *arrayref,
				    struct expression *array_index)
{
	struct expression *expr = alloc_expression(EXPR_ARRAY_DEREF, vm_type);
	if (expr) {
		expr->arrayref = &arrayref->node;
		expr->array_index = &array_index->node;
	}
	return expr;
}

struct expression *binop_expr(enum vm_type vm_type,
			      enum binary_operator binary_operator,
			      struct expression *binary_left, struct expression *binary_right)
{
	struct expression *expr = alloc_expression(EXPR_BINOP, vm_type);
	if (expr) {
		expr->node.op |= binary_operator << OP_SHIFT;
		expr->binary_left = &binary_left->node;
		expr->binary_right = &binary_right->node;
	}
	return expr;
}

struct expression *unary_op_expr(enum vm_type vm_type,
				 enum unary_operator unary_operator,
				 struct expression *unary_expression)
{
	struct expression *expr = alloc_expression(EXPR_UNARY_OP, vm_type);
	if (expr) {
		expr->node.op |= unary_operator << OP_SHIFT;
		expr->unary_expression = &unary_expression->node;
	}
	return expr;
}

struct expression *conversion_expr(enum vm_type vm_type,
				   struct expression *from_expression)
{
	struct expression *expr = alloc_expression(EXPR_CONVERSION, vm_type);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *class_field_expr(enum vm_type vm_type, struct fieldblock *class_field)
{
	struct expression *expr = alloc_expression(EXPR_CLASS_FIELD, vm_type);
	if (expr)
		expr->class_field = class_field;
	return expr;
}

struct expression *instance_field_expr(enum vm_type vm_type,
				       struct fieldblock *instance_field,
				       struct expression *objectref_expression)
{
	struct expression *expr = alloc_expression(EXPR_INSTANCE_FIELD, vm_type);
	if (expr) {
		expr->objectref_expression = &objectref_expression->node;
		expr->instance_field = instance_field;
	}
	return expr;
}

struct expression *__invoke_expr(enum expression_type expr_type, enum vm_type vm_type, struct methodblock *target_method)
{
	struct expression *expr = alloc_expression(expr_type, vm_type);

	if (expr)
		expr->target_method = target_method;

	return expr;
}

struct expression *invokevirtual_expr(struct methodblock *target)
{
	enum vm_type return_type;

	return_type = method_return_type(target);
	return __invoke_expr(EXPR_INVOKEVIRTUAL, return_type, target);
}

struct expression *invoke_expr(struct methodblock *target)
{
	enum vm_type return_type;

	return_type = method_return_type(target);
	return  __invoke_expr(EXPR_INVOKE, return_type, target);
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

struct expression *no_args_expr(void)
{
	return alloc_expression(EXPR_NO_ARGS, J_VOID);
}

struct expression *new_expr(struct object *class)
{
	struct expression *expr = alloc_expression(EXPR_NEW, J_REFERENCE);
	if (expr)
		expr->class = class;
	return expr;
}

struct expression *newarray_expr(unsigned long type, struct expression *size)
{
	struct expression *expr = alloc_expression(EXPR_NEWARRAY, J_REFERENCE);

	if (expr) {
		expr->array_type = type;
		expr->array_size = &size->node;
	}

	return expr;
}

struct expression *anewarray_expr(struct object *class, struct expression *size)
{
	struct expression *expr = alloc_expression(EXPR_ANEWARRAY, J_REFERENCE);

	if (expr) {
		expr->anewarray_ref_type = class;
		expr->anewarray_size = &size->node;
	}
	return expr;
}

struct expression *arraylength_expr(struct expression *arrayref)
{
	struct expression *expr = alloc_expression(EXPR_ARRAYLENGTH, J_REFERENCE);

	if (expr)
		expr->arraylength_ref = &arrayref->node;
	return expr;
}

unsigned long nr_args(struct expression *args_list)
{
	struct expression *left, *right;

	if (expr_type(args_list) == EXPR_NO_ARGS)
		return 0;

	if (expr_type(args_list) == EXPR_ARG)
		return 1;

	left  = to_expr(args_list->args_left);
	right = to_expr(args_list->args_right);

	return nr_args(left) + nr_args(right);
}
