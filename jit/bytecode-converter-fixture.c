/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <compilation-unit.h>
#include <basic-block.h>
#include <tree-node.h>
#include <expression.h>
#include <statement.h>
#include <jvm_types.h>
#include <libharness.h>

struct compilation_unit *alloc_simple_compilation_unit(unsigned char *code,
						       unsigned long code_len)
{
	struct compilation_unit *cu;
	struct basic_block *bb;

	cu = alloc_compilation_unit(code, code_len);
	bb = alloc_basic_block(0, code_len);
	list_add_tail(&bb->bb_list_node, &cu->bb_list);
	return cu;
}

void assert_value_expr(enum jvm_type expected_jvm_type,
		       long long expected_value, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_VALUE, expr_type(expr));
	assert_int_equals(expected_jvm_type, expr->jvm_type);
	assert_int_equals(expected_value, expr->value);
}

void assert_fvalue_expr(enum jvm_type expected_jvm_type,
			double expected_value, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_FVALUE, expr_type(expr));
	assert_int_equals(expected_jvm_type, expr->jvm_type);
	assert_float_equals(expected_value, expr->fvalue, 0.01f);
}

void assert_local_expr(enum jvm_type expected_jvm_type,
		       unsigned long expected_index, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_LOCAL, expr_type(expr));
	assert_int_equals(expected_jvm_type, expr->jvm_type);
	assert_int_equals(expected_index, expr->local_index);
}

void assert_temporary_expr(unsigned long expected, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_TEMPORARY, expr_type(expr));
	assert_int_equals(expected, expr->temporary);
}

void assert_array_deref_expr(enum jvm_type expected_jvm_type,
			     struct expression *expected_arrayref,
			     struct expression *expected_index,
			     struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_ARRAY_DEREF, expr_type(expr));
	assert_int_equals(expected_jvm_type, expr->jvm_type);
	assert_ptr_equals(expected_arrayref, to_expr(expr->arrayref));
	assert_ptr_equals(expected_index, to_expr(expr->array_index));
}

void __assert_binop_expr(enum jvm_type jvm_type,
			 enum binary_operator binary_operator,
			 struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_BINOP, expr_type(expr));
	assert_int_equals(jvm_type, expr->jvm_type);
	assert_int_equals(binary_operator, expr_bin_op(expr));
}

void assert_binop_expr(enum jvm_type jvm_type,
		       enum binary_operator binary_operator,
		       struct expression *binary_left,
		       struct expression *binary_right, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	__assert_binop_expr(jvm_type, binary_operator, node);
	assert_ptr_equals(binary_left, to_expr(expr->binary_left));
	assert_ptr_equals(binary_right, to_expr(expr->binary_right));
}

void assert_conv_expr(enum jvm_type expected_type,
		      struct expression *expected_expression,
		      struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_CONVERSION, expr_type(expr));
	assert_int_equals(expected_type, expr->jvm_type);
	assert_ptr_equals(expected_expression, to_expr(expr->from_expression));
}

void assert_field_expr(enum jvm_type expected_type,
		       struct fieldblock *expected_field,
		       struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_FIELD, expr_type(expr));
	assert_int_equals(expected_type, expr->jvm_type);
	assert_ptr_equals(expected_field, expr->field);
}

void assert_invoke_expr(enum jvm_type expected_type,
			struct methodblock *expected_method,
			struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_INVOKE, expr_type(expr));
	assert_int_equals(expected_type, expr->jvm_type);
	assert_ptr_equals(expected_method, expr->target_method);
}

void assert_store_stmt(struct statement *stmt)
{
	assert_int_equals(STMT_STORE, stmt_type(stmt));
}

void assert_return_stmt(struct expression *return_value, struct statement *stmt)
{
	assert_int_equals(STMT_RETURN, stmt_type(stmt));
	if (stmt->return_value)
		assert_ptr_equals(return_value, to_expr(stmt->return_value));
	else
		assert_ptr_equals(NULL, stmt->return_value);
}

void assert_null_check_stmt(struct expression *expected,
			    struct statement *actual)
{
	assert_int_equals(STMT_NULL_CHECK, stmt_type(actual));
	assert_value_expr(J_REFERENCE, expected->value, actual->expression);
}

void assert_arraycheck_stmt(enum jvm_type expected_jvm_type,
			    struct expression *expected_arrayref,
			    struct expression *expected_index,
			    struct statement *actual)
{
	assert_int_equals(STMT_ARRAY_CHECK, stmt_type(actual));
	assert_array_deref_expr(expected_jvm_type, expected_arrayref,
				expected_index, actual->expression);
}
