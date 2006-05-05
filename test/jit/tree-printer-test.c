/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/tree-printer.h>
#include <jvm_types.h>
#include <jit/statement.h>
#include <vm/string.h>

#include <libharness.h>
#include <stdlib.h>

static void assert_tree_print(const char *expected, struct tree_node *root)
{
	struct string *str = alloc_str();

	tree_print(root, str);
	assert_str_equals(expected, str->value);

	free_str(str);
}

static void assert_print_stmt(const char *expected, struct statement *stmt)
{
	assert_tree_print(expected, &stmt->node);
	free_statement(stmt);
}

void test_should_print_nop_statement(void)
{
	struct statement *stmt = alloc_statement(STMT_NOP);
	assert_print_stmt("NOP\n", stmt);
}

void test_should_print_store_statement(void)
{
	struct expression *dest, *src;
	struct statement *stmt;

	dest = local_expr(J_INT, 0);
	src = value_expr(J_INT, 1);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &dest->node;
	stmt->store_src = &src->node;

	assert_print_stmt("STORE:\n  store_dest: [local int 0]\n  store_src: [value int 1]\n", stmt);
}

void test_should_print_if_statement(void)
{
	struct string *expected;
	struct expression *if_conditional;
	struct statement *if_true, *stmt;

	if_conditional = local_expr(J_BOOLEAN, 0);
	if_true = alloc_statement(STMT_LABEL);

	stmt = alloc_statement(STMT_IF);
	stmt->if_conditional = &if_conditional->node;
	stmt->if_true = &if_true->node;

	expected = alloc_str();
	str_append(expected, "IF:\n  if_conditional: [local boolean 0]\n  if_true: [label %p]\n", if_true);
	assert_print_stmt(expected->value, stmt);

	free_statement(if_true);
	free_str(expected);
}

void test_should_print_label_statement(void)
{
	struct string *expected;
	struct statement *stmt;

	stmt = alloc_statement(STMT_LABEL);

	expected = alloc_str();
	str_append(expected, "LABEL: [%p]\n", stmt);
	assert_print_stmt(expected->value, stmt);
	free_str(expected);
}

void test_should_print_goto_statement(void)
{
	struct string *expected;
	struct statement *goto_target, *stmt;

	goto_target = alloc_statement(STMT_LABEL);
	stmt = alloc_statement(STMT_GOTO);
	stmt->goto_target = &goto_target->node;

	expected = alloc_str();
	str_append(expected, "GOTO:\n  goto_target: [label %p]\n", goto_target);
	assert_print_stmt(expected->value, stmt);

	free_statement(goto_target);
	free_str(expected);
}

void test_should_print_return_statement(void)
{
	struct expression *return_value;
	struct statement *stmt;

	return_value = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &return_value->node;

	assert_print_stmt("RETURN:\n  return_value: [local int 0]\n", stmt);
}

void test_should_print_void_return_statement(void)
{
	struct statement *stmt;

	stmt = alloc_statement(STMT_VOID_RETURN);

	assert_print_stmt("VOID_RETURN\n", stmt);
}

void test_should_print_expression_statement(void)
{
	struct expression *expression;
	struct statement *stmt;

	expression = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expression->node;

	assert_print_stmt("EXPRESSION:\n  expression: [local int 0]\n", stmt);
}

void test_should_print_nullcheck_statement(void)
{
	struct expression *expression;
	struct statement *stmt;

	expression = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_NULL_CHECK);
	stmt->expression = &expression->node;

	assert_print_stmt("NULL_CHECK:\n  expression: [local int 0]\n", stmt);
}

void test_should_print_arraycheck_statement(void)
{
	struct expression *expression;
	struct statement *stmt;

	expression = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_ARRAY_CHECK);
	stmt->expression = &expression->node;

	assert_print_stmt("ARRAY_CHECK:\n  expression: [local int 0]\n", stmt);
}

static void assert_print_expr(const char *expected, struct expression *expr)
{
	assert_tree_print(expected, &expr->node);
	expr_put(expr);
}

void assert_printed_value_expr(const char *expected, enum jvm_type type,
			       unsigned long long value)
{
	struct expression *expr;

	expr = value_expr(type, value);
	assert_print_expr(expected, expr);
}

void test_should_print_value_expression(void)
{
	assert_printed_value_expr("[value int 0]", J_INT, 0);
	assert_printed_value_expr("[value boolean 1]", J_BOOLEAN, 1);
}

void assert_printed_fvalue_expr(const char *expected, enum jvm_type type,
			        double fvalue)
{
	struct expression *expr;

	expr = fvalue_expr(type, fvalue);
	assert_print_expr(expected, expr);
}

void test_should_print_fvalue_expression(void)
{
	assert_printed_fvalue_expr("[fvalue float 0.000000]", J_FLOAT, 0.0);
	assert_printed_fvalue_expr("[fvalue double 1.100000]", J_DOUBLE, 1.1);
}

void assert_printed_local_expr(const char *expected, enum jvm_type type,
			       unsigned long local_index)
{
	struct expression *expr;

	expr = local_expr(type, local_index);
	assert_print_expr(expected, expr);
}

void test_should_print_local_expression(void)
{
	assert_printed_local_expr("[local int 0]", J_INT, 0);
	assert_printed_local_expr("[local boolean 1]", J_BOOLEAN, 1);
}

void assert_printed_temporary_expr(const char *expected, enum jvm_type type,
				   unsigned long temporary)
{
	struct expression *expr;

	expr = temporary_expr(type, temporary);
	assert_print_expr(expected, expr);
}

void test_should_print_temporary_expression(void)
{
	assert_printed_temporary_expr("[temporary int 0]", J_INT, 0);
	assert_printed_temporary_expr("[temporary boolean 1]", J_BOOLEAN, 1);
}

void assert_printed_array_deref_expr(const char *expected, enum jvm_type type,
				     unsigned long arrayref, unsigned long array_index)
{
	struct expression *expr;

	expr = array_deref_expr(type,
			value_expr(J_REFERENCE, arrayref),
			value_expr(J_INT, array_index));
	assert_print_expr(expected, expr);
}

void test_should_print_array_deref_expression(void)
{
	assert_printed_array_deref_expr("ARRAY_DEREF:\n  jvm_type: [float]\n  arrayref: [value reference 0]\n  array_index: [value int 1]\n", J_FLOAT, 0, 1);
	assert_printed_array_deref_expr("ARRAY_DEREF:\n  jvm_type: [double]\n  arrayref: [value reference 1]\n  array_index: [value int 2]\n", J_DOUBLE, 1, 2);
}

void assert_printed_binop_expr(const char *expected, enum jvm_type type,
			       enum binary_operator op,
			       struct expression *binary_left,
			       struct expression *binary_right)
{
	struct expression *expr;

	expr = binop_expr(type, op, binary_left, binary_right);
	assert_print_expr(expected, expr);
}

void test_should_print_binop_expression(void)
{
	assert_printed_binop_expr("BINOP:\n"
				  "  jvm_type: [int]\n"
				  "  binary_operator: [add]\n"
				  "  binary_left: [value int 0]\n"
				  "  binary_right: [value int 1]\n",
				  J_INT, OP_ADD,
				  value_expr(J_INT, 0), value_expr(J_INT, 1));

	assert_printed_binop_expr("BINOP:\n"
				  "  jvm_type: [long]\n"
				  "  binary_operator: [add]\n"
				  "  binary_left: [value long 1]\n"
				  "  binary_right:\n"
				  "    BINOP:\n"
				  "      jvm_type: [long]\n"
				  "      binary_operator: [sub]\n"
				  "      binary_left: [value long 2]\n"
				  "      binary_right: [value long 3]\n",
				  J_LONG, OP_ADD,
				  value_expr(J_LONG, 1),
				  binop_expr(J_LONG, OP_SUB,
					  value_expr(J_LONG, 2),
					  value_expr(J_LONG, 3))
				  );
}

void assert_printed_unary_op_expr(const char *expected, enum jvm_type type,
				  enum unary_operator op,
				  struct expression *unary_expr)
{
	struct expression *expr;

	expr = unary_op_expr(type, op, unary_expr);
	assert_print_expr(expected, expr);
}

void test_should_print_unary_op_expression(void)
{
	assert_printed_unary_op_expr("UNARY_OP:\n"
				     "  jvm_type: [int]\n"
				     "  unary_operator: [neg]\n"
				     "  unary_expression: [value int 0]\n",
				     J_INT, OP_NEG, value_expr(J_INT, 0));

	assert_printed_unary_op_expr("UNARY_OP:\n"
				     "  jvm_type: [boolean]\n"
				     "  unary_operator: [neg]\n"
				     "  unary_expression: [value boolean 1]\n",
				     J_BOOLEAN, OP_NEG, value_expr(J_BOOLEAN, 1));
}

void assert_printed_conversion_expr(const char *expected, enum jvm_type type,
				    struct expression *from_expr)
{
	struct expression *expr;

	expr = conversion_expr(type, from_expr);
	assert_print_expr(expected, expr);
}

void test_should_print_conversion_expression(void)
{
	assert_printed_conversion_expr("CONVERSION:\n"
				     "  jvm_type: [long]\n"
				     "  from_expression: [value int 0]\n",
				     J_LONG, value_expr(J_INT, 0));

	assert_printed_conversion_expr("CONVERSION:\n"
				     "  jvm_type: [int]\n"
				     "  from_expression: [value boolean 1]\n",
				     J_INT, value_expr(J_BOOLEAN, 1));
}

void assert_printed_field_expr(const char *expected, enum jvm_type type,
			       struct fieldblock *field)
{
	struct expression *expr;

	expr = field_expr(type, field);
	assert_print_expr(expected, expr);
}

void test_should_print_field_expression(void)
{
	struct fieldblock fb;
	struct string *expected;

	expected = alloc_str();
	str_append(expected, "[field int %p]", &fb);

	assert_printed_field_expr(expected->value, J_INT, &fb);
	free_str(expected);
}

void assert_printed_invoke_expr(const char *expected, enum jvm_type type,
				struct methodblock *method,
				struct expression *args_list)
{
	struct expression *expr;

	expr = invoke_expr(type, method);
	expr->args_list = &args_list->node;
	assert_print_expr(expected, expr);
}

void test_should_print_invoke_expression(void)
{
	struct methodblock mb;
	struct string *expected;

	expected = alloc_str();
	str_append(expected, "INVOKE:\n"
			     "  target_method: [%p]\n"
			     "  args_list: [no args]\n",
			     &mb);

	assert_printed_invoke_expr(expected->value, J_INT, &mb, no_args_expr());
	free_str(expected);
}

void assert_printed_args_list_expr(const char *expected,
				   struct expression *args_left,
				   struct expression *args_right)
{
	struct expression *expr;

	expr = args_list_expr(arg_expr(args_left), arg_expr(args_right));
	assert_print_expr(expected, expr);
}

void test_should_print_args_list_expression(void)
{
	assert_printed_args_list_expr("ARGS_LIST:\n"
				     "  args_left:\n"
				     "    ARG:\n"
				     "      arg_expression: [value int 0]\n"
				     "  args_right:\n"
				     "    ARG:\n"
				     "      arg_expression: [value boolean 1]\n",
				     value_expr(J_INT, 0),
				     value_expr(J_BOOLEAN, 1));
}

void assert_printed_arg_expr(const char *expected,
			     struct expression *arg_expression)
{
	struct expression *expr;

	expr = arg_expr(arg_expression);
	assert_print_expr(expected, expr);
}

void test_should_print_arg_expression(void)
{
	assert_printed_arg_expr("ARG:\n"
				     "  arg_expression: [value int 0]\n",
				     value_expr(J_INT, 0));

	assert_printed_arg_expr("ARG:\n"
				     "  arg_expression: [value boolean 1]\n",
				     value_expr(J_BOOLEAN, 1));
}

void test_should_print_no_args_expression(void)
{
	assert_print_expr("[no args]", no_args_expr());
}
