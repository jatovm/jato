/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/statement.h"
#include "jit/expression.h"
#include "jit/bc-offset-mapping.h"

#include "vm/backtrace.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/die.h"
#include "vm/vm.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* How many child expressions are used by each type of expression. */
int expr_nr_kids(struct expression *expr)
{
	switch (expr_type(expr)) {
	case EXPR_ARRAY_DEREF:
	case EXPR_BINOP:
	case EXPR_ARGS_LIST:
		return 2;
	case EXPR_UNARY_OP:
	case EXPR_TRUNCATION:
	case EXPR_CONVERSION:
	case EXPR_CONVERSION_FLOAT_TO_DOUBLE:
	case EXPR_CONVERSION_DOUBLE_TO_FLOAT:
	case EXPR_CONVERSION_TO_FLOAT:
	case EXPR_CONVERSION_FROM_FLOAT:
	case EXPR_CONVERSION_TO_DOUBLE:
	case EXPR_CONVERSION_FROM_DOUBLE:
	case EXPR_INSTANCE_FIELD:
	case EXPR_FLOAT_INSTANCE_FIELD:
	case EXPR_ARG:
	case EXPR_ARG_THIS:
	case EXPR_NEWARRAY:
	case EXPR_ANEWARRAY:
	case EXPR_MULTIANEWARRAY:
	case EXPR_ARRAYLENGTH:
	case EXPR_INSTANCEOF:
	case EXPR_NULL_CHECK:
	case EXPR_ARRAY_SIZE_CHECK:
	case EXPR_LOOKUPSWITCH_BSEARCH:
		return 1;
	case EXPR_VALUE:
	case EXPR_FLOAT_LOCAL:
	case EXPR_FLOAT_TEMPORARY:
	case EXPR_FLOAT_CLASS_FIELD:
	case EXPR_FVALUE:
	case EXPR_LOCAL:
	case EXPR_TEMPORARY:
	case EXPR_CLASS_FIELD:
	case EXPR_NO_ARGS:
	case EXPR_NEW:
	case EXPR_EXCEPTION_REF:
	case EXPR_MIMIC_STACK_SLOT:
		return 0;
	default:
		assert(!"Invalid expression type");
	}
}

/**
 * Returns true if given expression does not have any potential side
 * effects and hence one instance of this expression can be safely
 * connected to many nodes in the tree.
 */
int expr_is_pure(struct expression *expr)
{
	int i;

	switch (expr_type(expr)) {
		/*
		 * These expressions may or may not have side effects
		 * depending on their actual children expressions. In
		 * general these expressions should not be connected
		 * to more than one node.
		 */
	case EXPR_ARGS_LIST:
	case EXPR_UNARY_OP:
	case EXPR_TRUNCATION:
	case EXPR_CONVERSION:
	case EXPR_CONVERSION_FLOAT_TO_DOUBLE:
	case EXPR_CONVERSION_DOUBLE_TO_FLOAT:
	case EXPR_CONVERSION_TO_FLOAT:
	case EXPR_CONVERSION_FROM_FLOAT:
	case EXPR_CONVERSION_TO_DOUBLE:
	case EXPR_CONVERSION_FROM_DOUBLE:
	case EXPR_ARG:
	case EXPR_ARG_THIS:
	case EXPR_ARRAYLENGTH:
	case EXPR_INSTANCEOF:
	case EXPR_ARRAY_SIZE_CHECK:
	case EXPR_NULL_CHECK:
	case EXPR_LOOKUPSWITCH_BSEARCH:
	case EXPR_INSTANCE_FIELD:
	case EXPR_FLOAT_INSTANCE_FIELD:

		/* These expression types should be always assumed to
		   have side-effects. */
	case EXPR_NEWARRAY:
	case EXPR_ANEWARRAY:
	case EXPR_MULTIANEWARRAY:
	case EXPR_NEW:
	case EXPR_BINOP:
		return false;

		/* These expression types do not have any side-effects */
	case EXPR_VALUE:
	case EXPR_FVALUE:
	case EXPR_LOCAL:
	case EXPR_FLOAT_LOCAL:
	case EXPR_TEMPORARY:
	case EXPR_FLOAT_TEMPORARY:
	case EXPR_CLASS_FIELD:
	case EXPR_FLOAT_CLASS_FIELD:
	case EXPR_NO_ARGS:
	case EXPR_EXCEPTION_REF:
	case EXPR_MIMIC_STACK_SLOT:
		return true;

		/* EXPR_ARRAY_DEREF can have side effects in general
		   but it can not be copied so it's considered pure
		   when all it's children are pure. */
	case EXPR_ARRAY_DEREF:
		for (i = 0; i < expr_nr_kids(expr); i++)
			if (!expr_is_pure(to_expr(expr->node.kids[i])))
				return false;
		return true;

	default:
		error("Invalid expression type: %d", expr_type(expr));
	}
}

struct expression *alloc_expression(enum expression_type type,
				    enum vm_type vm_type)
{
	struct expression *expr = malloc(sizeof *expr);
	if (expr) {
		memset(expr, 0, sizeof *expr);
		expr->node.op = type << EXPR_TYPE_SHIFT;
		expr->vm_type = vm_type;
		expr->refcount = 1;
		expr->node.bytecode_offset = BC_OFFSET_UNKNOWN;
	}
	return expr;
}

void free_expression(struct expression *expr)
{
	int i;

	if (!expr)
		return;

	for (i = 0; i < expr_nr_kids(expr); i++)
		if (expr->node.kids[i])
			expr_put(to_expr(expr->node.kids[i]));

	free(expr);
}

struct expression *expr_get(struct expression *expr)
{
	assert(expr->refcount > 0);

	if (!expr_is_pure(expr))
		error("expression is not pure");

	expr->refcount++;

	return expr;
}

void expr_put(struct expression *expr)
{
	if (!(expr->refcount > 0))
		print_trace();
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
	struct expression *expr;

	if (vm_type_is_float(vm_type))
		expr = alloc_expression(EXPR_FLOAT_LOCAL, vm_type);
	else
		expr = alloc_expression(EXPR_LOCAL, vm_type);

	if (expr)
		expr->local_index = local_index;

	return expr;
}

struct expression *temporary_expr(enum vm_type vm_type, struct compilation_unit *cu)
{
	struct expression *expr;

	if (vm_type_is_float(vm_type))
		expr = alloc_expression(EXPR_FLOAT_TEMPORARY, vm_type);
	else
		expr = alloc_expression(EXPR_TEMPORARY, vm_type);

	if (!expr)
		return NULL;

	if (expr->vm_type == J_LONG) {
#ifdef CONFIG_32_BIT
		expr->tmp_low = get_var(cu, J_INT);
		expr->tmp_high = get_var(cu, J_INT);
#else
		expr->tmp_low = get_var(cu, J_LONG);
#endif
	} else {
		expr->tmp_low = get_var(cu, expr->vm_type);
#ifdef CONFIG_32_BIT
		expr->tmp_high = NULL;
#endif
	}

	return expr;
}

struct expression *mimic_stack_expr(enum vm_type vm_type, int entry, int slot_ndx)
{
	struct expression *expr = alloc_expression(EXPR_MIMIC_STACK_SLOT, vm_type);
	if (expr) {
		expr->entry = entry;
		expr->slot_ndx = slot_ndx;
	}

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

struct expression *
conversion_double_to_float_expr(struct expression *from_expression)
{
	struct expression *expr;

	expr = alloc_expression(EXPR_CONVERSION_DOUBLE_TO_FLOAT, J_FLOAT);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *
conversion_float_to_double_expr(struct expression *from_expression)
{
	struct expression *expr;

	expr = alloc_expression(EXPR_CONVERSION_FLOAT_TO_DOUBLE, J_DOUBLE);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *conversion_to_float_expr(enum vm_type vm_type,
				   struct expression *from_expression)
{
	struct expression *expr = alloc_expression(EXPR_CONVERSION_TO_FLOAT, vm_type);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *conversion_from_float_expr(enum vm_type vm_type,
				   struct expression *from_expression)
{
	struct expression *expr = alloc_expression(EXPR_CONVERSION_FROM_FLOAT, vm_type);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *conversion_to_double_expr(enum vm_type vm_type,
					     struct expression *from_expression)
{
	struct expression *expr = alloc_expression(EXPR_CONVERSION_TO_DOUBLE, vm_type);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *conversion_from_double_expr(enum vm_type vm_type,
					struct expression *from_expression)
{
	struct expression *expr = alloc_expression(EXPR_CONVERSION_FROM_DOUBLE,
						   vm_type);
	if (expr)
		expr->from_expression = &from_expression->node;
	return expr;
}

struct expression *class_field_expr(enum vm_type vm_type, struct vm_field *class_field)
{
	struct expression *expr;

	if (vm_type_is_float(vm_type))
		expr = alloc_expression(EXPR_FLOAT_CLASS_FIELD, vm_type);
	else
		expr = alloc_expression(EXPR_CLASS_FIELD, vm_type);

	if (expr)
		expr->class_field = class_field;
	return expr;
}

struct expression *instance_field_expr(enum vm_type vm_type,
				       struct vm_field *instance_field,
				       struct expression *objectref_expression)
{
	struct expression *expr;

	if (vm_type_is_float(vm_type))
		expr = alloc_expression(EXPR_FLOAT_INSTANCE_FIELD, vm_type);
	else
		expr = alloc_expression(EXPR_INSTANCE_FIELD, vm_type);

	if (expr) {
		expr->objectref_expression = &objectref_expression->node;
		expr->instance_field = instance_field;
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
	struct expression *expr = alloc_expression(EXPR_ARG, arg_expression->vm_type);
	if (expr)
		expr->arg_expression = &arg_expression->node;
	return expr;
}

struct expression *arg_this_expr(struct expression *arg_expression)
{
	struct expression *expr = alloc_expression(EXPR_ARG_THIS, J_REFERENCE);
	if (expr)
		expr->arg_expression = &arg_expression->node;
	return expr;
}

struct expression *no_args_expr(void)
{
	return alloc_expression(EXPR_NO_ARGS, J_VOID);
}

struct expression *new_expr(struct vm_class *class)
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

struct expression *anewarray_expr(struct vm_class *class, struct expression *size)
{
	struct expression *expr = alloc_expression(EXPR_ANEWARRAY, J_REFERENCE);

	if (expr) {
		expr->anewarray_ref_type = class;
		expr->anewarray_size = &size->node;
	}
	return expr;
}

struct expression *multianewarray_expr(struct vm_class *class)
{
	struct expression *expr = alloc_expression(EXPR_MULTIANEWARRAY, J_REFERENCE);

	if (expr)
		expr->multianewarray_ref_type = class;
	return expr;
}

struct expression *arraylength_expr(struct expression *arrayref)
{
	struct expression *expr = alloc_expression(EXPR_ARRAYLENGTH, J_INT);

	if (expr)
		expr->arraylength_ref = &arrayref->node;
	return expr;
}

struct expression *instanceof_expr(struct expression *objectref, struct vm_class *class)
{
	struct expression *expr = alloc_expression(EXPR_INSTANCEOF, J_INT);

	if (expr) {
		expr->instanceof_class = class;
		expr->instanceof_ref = &objectref->node;
	}
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

struct expression *exception_ref_expr(void)
{
	return alloc_expression(EXPR_EXCEPTION_REF, J_REFERENCE);
}

struct expression *null_check_expr(struct expression *ref)
{
	struct expression *expr;

	expr = alloc_expression(EXPR_NULL_CHECK, J_REFERENCE);
	if (!expr)
		return NULL;

	assert(ref->vm_type == J_REFERENCE);

	expr->bytecode_offset = ref->bytecode_offset;
	expr->null_check_ref = &ref->node;

	return expr;
}

struct expression *array_size_check_expr(struct expression *size)
{
	struct expression *expr;

	expr = alloc_expression(EXPR_ARRAY_SIZE_CHECK, J_INT);
	if (!expr)
		return NULL;

	expr->size_expr = &size->node;

	return expr;
}

struct expression *lookupswitch_bsearch_expr(struct expression *key, struct lookupswitch *table)
{
	struct expression *expr;

	expr = alloc_expression(EXPR_LOOKUPSWITCH_BSEARCH, J_NATIVE_PTR);
	if (!expr)
		return NULL;

	expr->key = &key->node;
	expr->lookupswitch_table = table;

	return expr;
}

struct expression *truncation_expr(enum vm_type to_type,
				   struct expression *from_expression)
{
	struct expression *expr;

	expr = alloc_expression(EXPR_TRUNCATION, J_INT);
	if (!expr)
		return NULL;

	expr->from_expression = &from_expression->node;
	expr->to_type = to_type;

	return expr;
}
