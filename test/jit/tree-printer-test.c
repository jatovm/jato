/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/tree-printer.h>
#include <jit/statement.h>
#include <vm/string.h>
#include <vm/types.h>

#include <libharness.h>
#include <stdlib.h>

#include <test/vm.h>

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

void test_should_print_store_statement(void)
{
	struct expression *dest, *src;
	struct statement *stmt;

	dest = local_expr(J_INT, 0);
	src = value_expr(J_INT, 1);

	stmt = alloc_statement(STMT_STORE);
	stmt->store_dest = &dest->node;
	stmt->store_src = &src->node;

	assert_print_stmt("STORE:\n  store_dest: [local int 0]\n  store_src: [value int 0x1]\n", stmt);
}

void test_should_print_if_statement(void)
{
	struct string *expected;
	struct expression *if_conditional;
	struct statement *stmt;
	struct basic_block *if_true = (void *) 0xdeadbeef;

	if_conditional = local_expr(J_BOOLEAN, 0);

	stmt = alloc_statement(STMT_IF);
	stmt->if_conditional = &if_conditional->node;
	stmt->if_true = if_true;

	expected = alloc_str();
	str_append(expected, "IF:\n  if_conditional: [local boolean 0]\n  if_true: [bb %p]\n", if_true);
	assert_print_stmt(expected->value, stmt);

	free_str(expected);
}

void test_should_print_goto_statement(void)
{
	struct string *expected;
	struct statement *stmt;
	struct basic_block *goto_target = (void *) 0xdeadbeef;

	stmt = alloc_statement(STMT_GOTO);
	stmt->goto_target = goto_target;

	expected = alloc_str();
	str_append(expected, "GOTO:\n  goto_target: [bb %p]\n", goto_target);
	assert_print_stmt(expected->value, stmt);

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

void assert_printed_value_expr(const char *expected, enum vm_type type,
			       unsigned long long value)
{
	struct expression *expr;

	expr = value_expr(type, value);
	assert_print_expr(expected, expr);
}

void test_should_print_value_expression(void)
{
	assert_printed_value_expr("[value int 0x0]", J_INT, 0);
	assert_printed_value_expr("[value boolean 0x1]", J_BOOLEAN, 1);
}

void assert_printed_fvalue_expr(const char *expected, enum vm_type type,
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

void assert_printed_local_expr(const char *expected, enum vm_type type,
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

void assert_printed_temporary_expr(const char *expected, enum vm_type type, struct var_info *var)
{
	struct expression *expr;

	expr = temporary_expr(type, var);
	assert_print_expr(expected, expr);
}

void test_should_print_temporary_expression(void)
{
	assert_printed_temporary_expr("[temporary int]", J_INT, (struct var_info *)0x12345678);
	assert_printed_temporary_expr("[temporary boolean]", J_BOOLEAN, (struct var_info *)0x87654321);
}

void assert_printed_array_deref_expr(const char *expected, enum vm_type type,
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
	assert_printed_array_deref_expr("ARRAY_DEREF:\n  vm_type: [float]\n  arrayref: [value reference 0x0]\n  array_index: [value int 0x1]\n", J_FLOAT, 0, 1);
	assert_printed_array_deref_expr("ARRAY_DEREF:\n  vm_type: [double]\n  arrayref: [value reference 0x1]\n  array_index: [value int 0x2]\n", J_DOUBLE, 1, 2);
}

void assert_printed_binop_expr(const char *expected, enum vm_type type,
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
				  "  vm_type: [int]\n"
				  "  binary_operator: [add]\n"
				  "  binary_left: [value int 0x0]\n"
				  "  binary_right: [value int 0x1]\n",
				  J_INT, OP_ADD,
				  value_expr(J_INT, 0), value_expr(J_INT, 1));

	assert_printed_binop_expr("BINOP:\n"
				  "  vm_type: [long]\n"
				  "  binary_operator: [add]\n"
				  "  binary_left: [value long 0x1]\n"
				  "  binary_right:\n"
				  "    BINOP:\n"
				  "      vm_type: [long]\n"
				  "      binary_operator: [sub]\n"
				  "      binary_left: [value long 0x2]\n"
				  "      binary_right: [value long 0x3]\n",
				  J_LONG, OP_ADD,
				  value_expr(J_LONG, 1),
				  binop_expr(J_LONG, OP_SUB,
					  value_expr(J_LONG, 2),
					  value_expr(J_LONG, 3))
				  );
}

void assert_printed_unary_op_expr(const char *expected, enum vm_type type,
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
				     "  vm_type: [int]\n"
				     "  unary_operator: [neg]\n"
				     "  unary_expression: [value int 0x0]\n",
				     J_INT, OP_NEG, value_expr(J_INT, 0));

	assert_printed_unary_op_expr("UNARY_OP:\n"
				     "  vm_type: [boolean]\n"
				     "  unary_operator: [neg]\n"
				     "  unary_expression: [value boolean 0x1]\n",
				     J_BOOLEAN, OP_NEG, value_expr(J_BOOLEAN, 1));
}

void assert_printed_conversion_expr(const char *expected, enum vm_type type,
				    struct expression *from_expr)
{
	struct expression *expr;

	expr = conversion_expr(type, from_expr);
	assert_print_expr(expected, expr);
}

void test_should_print_conversion_expression(void)
{
	assert_printed_conversion_expr("CONVERSION:\n"
				     "  vm_type: [long]\n"
				     "  from_expression: [value int 0x0]\n",
				     J_LONG, value_expr(J_INT, 0));

	assert_printed_conversion_expr("CONVERSION:\n"
				     "  vm_type: [int]\n"
				     "  from_expression: [value boolean 0x1]\n",
				     J_INT, value_expr(J_BOOLEAN, 1));
}

void assert_printed_class_field_expr(const char *expected, enum vm_type type,
				     struct fieldblock *field)
{
	struct expression *expr;

	expr = class_field_expr(type, field);
	assert_print_expr(expected, expr);
}

void test_should_print_class_field_expression(void)
{
	struct string *expected;
	struct fieldblock fb;

	expected = alloc_str();
	str_append(expected, "[class_field int %p]", &fb);

	assert_printed_class_field_expr(expected->value, J_INT, &fb);
	free_str(expected);
}

void assert_printed_instance_field_expr(const char *expected, enum vm_type type,
					struct fieldblock *field,
					struct expression *objectref)
{
	struct expression *expr;

	expr = instance_field_expr(type, field, objectref);
	assert_print_expr(expected, expr);
}

void test_should_print_instance_field_expression(void)
{
	struct expression *objectref;
	struct string *expected;
	struct fieldblock fb;

	objectref = value_expr(J_REFERENCE, 0xdeadbeef);
	expected = alloc_str();
	str_append(expected, "INSTANCE_FIELD:\n"
			     "  vm_type: [int]\n"
			     "  instance_field: [%p]\n"
			     "  objectref_expression: [value reference 0xdeadbeef]\n",
			     &fb);

	assert_printed_instance_field_expr(expected->value, J_INT, &fb, objectref);
	free_str(expected);
}

void assert_printed_invoke_expr(const char *expected,
				struct methodblock *method,
				struct expression *args_list)
{
	struct expression *expr;

	expr = invoke_expr(method);
	expr->args_list = &args_list->node;
	assert_print_expr(expected, expr);
}

void test_should_print_invoke_expression(void)
{	
	struct object *class = new_class();
	struct methodblock method = {
		.class = class,
		.name = "getId",
		.type = "()I",
	};
	struct string *expected;

	CLASS_CB(class)->name = "Foo";

	expected = alloc_str();
	str_append(expected, "INVOKE:\n"
			     "  target_method: [%p 'Foo.getId()I']\n"
			     "  args_list: [no args]\n",
			     &method);

	assert_printed_invoke_expr(expected->value, &method, no_args_expr());
	free_str(expected);
	free(class);
}

void assert_printed_invokevirtual_expr(const char *expected,
				       struct methodblock *target,
				       struct expression *args_list)
{
	struct expression *expr;

	expr = invokevirtual_expr(target);
	expr->args_list = &args_list->node;
	assert_print_expr(expected, expr);
}

void test_should_print_invokevirtual_expression(void)
{
	struct object *class = new_class();
	struct methodblock method = {
		.class = class,
		.method_table_index = 0xdeadbeef,
		.name = "bar",
		.type = "(I)V",
	};
	struct string *expected;

	CLASS_CB(class)->name = "Foo";

	expected = alloc_str();
	str_append(expected, "INVOKEVIRTUAL:\n"
			     "  target_method: [%p 'Foo.bar(I)V' (%lu)]\n"
			     "  args_list: [no args]\n",
			     &method, 0xdeadbeef);

	assert_printed_invokevirtual_expr(expected->value, &method, no_args_expr());
	free_str(expected);
	free(class);
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
				     "      arg_expression: [value int 0x0]\n"
				     "  args_right:\n"
				     "    ARG:\n"
				     "      arg_expression: [value boolean 0x1]\n",
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
				     "  arg_expression: [value int 0x0]\n",
				     value_expr(J_INT, 0));

	assert_printed_arg_expr("ARG:\n"
				     "  arg_expression: [value boolean 0x1]\n",
				     value_expr(J_BOOLEAN, 1));
}

void test_should_print_no_args_expression(void)
{
	assert_print_expr("[no args]", no_args_expr());
}

void assert_printed_new_expr(const char *expected, struct object *class)
{
	struct expression *expr;

	expr = new_expr(class);
	assert_print_expr(expected, expr);
}

void test_should_print_new_expression(void)
{
	assert_printed_new_expr("NEW:\n"
			     "  vm_type: [reference]\n"
			     "  class: [0xcafe]\n",
			     (void *) 0xcafe);
}

void test_print_newarray_expression(void)
{
	assert_print_expr("NEWARRAY:\n"
			  "  vm_type: [reference]\n"
			  "  array_size: [value int 0xff]\n"
			  "  array_type: [10]\n",
			  newarray_expr(T_INT, value_expr(J_INT, 0xff)));
}

void test_print_anewarray_expression(void)
{
	assert_print_expr("ANEWARRAY:\n"
			  "  vm_type: [reference]\n"
			  "  anewarray_size: [value int 0xff]\n"
			  "  anewarray_ref_type: [0x55]\n",
			  anewarray_expr((void *) 0x55, value_expr(J_INT, 0xff)));
}

void test_print_multianewarray_expression(void)
{
	struct expression *expr, *args_list;
	args_list = args_list_expr(arg_expr(value_expr(J_INT, 0x02)),
				   arg_expr(value_expr(J_INT, 0xff)));

	expr = multianewarray_expr((void *) 0xff);
	expr->multianewarray_dimensions = &args_list->node;

	assert_print_expr("MULTIANEWARRAY:\n"
			  "  vm_type: [reference]\n"
			  "  multianewarray_ref_type: [0xff]\n"
			  "  dimension list:\n"
			  "    ARGS_LIST:\n"
			  "      args_left:\n"
			  "        ARG:\n"
			  "          arg_expression: [value int 0x2]\n"
			  "      args_right:\n"
			  "        ARG:\n"
			  "          arg_expression: [value int 0xff]\n",
			  expr);
}

void test_print_arraylength_expression(void)
{
	assert_print_expr("ARRAYLENGTH:\n"
			  "  vm_type: [reference]\n"
			  "  arraylength_ref: [value reference 0xcafe]\n",
			  arraylength_expr(value_expr(J_REFERENCE, 0xcafe)));
}

void test_print_instanceof_expression(void)
{
	assert_print_expr("INSTANCEOF:\n"
			  "  vm_type: [reference]\n"
			  "  instanceof_class: [0x55]\n"
			  "  instanceof_ref: [value reference 0x55]\n",
			  instanceof_expr(value_expr(J_REFERENCE, 0x55), (void *)0x55));
}
