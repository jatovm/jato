/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include "jit/tree-printer.h"
#include "jit/statement.h"
#include "vm/class.h"
#include "vm/field.h"
#include "vm/method.h"
#include "lib/string.h"
#include "vm/types.h"

#include <libharness.h>
#include <stdlib.h>

#include "test/vm.h"

static struct vm_class vmc = {
	.name = "Class",
};

static struct vm_field vmf = {
	.class = &vmc,
	.name = "field",
	.type_info = { .vm_type = J_INT, .class_name = "I" }
};

static struct vm_method vmm = {
	.class = &vmc,
	.virtual_index = 255,
	.name = "method",
	.type ="()I",
};

struct string *str_aprintf(const char *fmt, ...)
{
	va_list ap;
	struct string *str;

	str = alloc_str();

	va_start(ap, fmt);
	str_vappend(str, fmt, ap);
	va_end(ap);

	return str;
}

static void assert_tree_print(struct string *expected, struct tree_node *root)
{
	struct string *str = alloc_str();

	tree_print(root, str);
	assert_str_equals(expected->value, str->value);

	free_str(str);
	free_str(expected);
}

static void assert_print_stmt(struct string *expected, struct statement *stmt)
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

	assert_print_stmt(str_aprintf(
		"STORE:\n  store_dest: [local int 0]\n"
		"  store_src: [value int 0x1]\n"), stmt);
}

void test_should_print_if_statement(void)
{
	struct expression *if_conditional;
	struct statement *stmt;
	struct basic_block *if_true = (void *) 0xdeadbeef;

	if_conditional = local_expr(J_BOOLEAN, 0);

	stmt = alloc_statement(STMT_IF);
	stmt->if_conditional = &if_conditional->node;
	stmt->if_true = if_true;

	assert_print_stmt(str_aprintf(
		"IF:\n"
		"  if_conditional: [local boolean 0]\n"
		"  if_true: [bb %p]\n", if_true), stmt);
}

void test_should_print_goto_statement(void)
{
	struct statement *stmt;
	struct basic_block *goto_target = (void *) 0xdeadbeef;

	stmt = alloc_statement(STMT_GOTO);
	stmt->goto_target = goto_target;

	assert_print_stmt(str_aprintf(
		"GOTO:\n"
		"  goto_target: [bb %p]\n", goto_target), stmt);
}

void test_should_print_return_statement(void)
{
	struct expression *return_value;
	struct statement *stmt;

	return_value = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &return_value->node;

	assert_print_stmt(str_aprintf(
		"RETURN:\n"
		"  return_value: [local int 0]\n"), stmt);
}

void test_should_print_void_return_statement(void)
{
	struct statement *stmt;

	stmt = alloc_statement(STMT_VOID_RETURN);

	assert_print_stmt(str_aprintf("VOID_RETURN\n"), stmt);
}

void test_should_print_expression_statement(void)
{
	struct expression *expression;
	struct statement *stmt;

	expression = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expression->node;

	assert_print_stmt(str_aprintf(
		"EXPRESSION:\n"
		"  expression: [local int 0]\n"), stmt);
}

void test_should_print_arraycheck_statement(void)
{
	struct expression *expression;
	struct statement *stmt;

	expression = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_ARRAY_CHECK);
	stmt->expression = &expression->node;

	assert_print_stmt(str_aprintf(
		"ARRAY_CHECK:\n"
		"  expression: [local int 0]\n"), stmt);
}

void test_should_print_monitorenter_statement(void)
{
	struct expression *expr;
	struct statement *stmt;

	expr = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_MONITOR_ENTER);
	stmt->expression = &expr->node;

	assert_print_stmt(str_aprintf(
		"MONITOR_ENTER:\n"
		"  expression: [local int 0]\n"), stmt);
}

void test_should_print_monitorexit_statement(void)
{
	struct expression *expr;
	struct statement *stmt;

	expr = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_MONITOR_EXIT);
	stmt->expression = &expr->node;

	assert_print_stmt(str_aprintf(
		"MONITOR_EXIT:\n"
		"  expression: [local int 0]\n"), stmt);
}

void test_should_print_checkcast_statement(void)
{
	struct expression *expr;
	struct statement *stmt;

	expr = local_expr(J_INT, 0);

	stmt = alloc_statement(STMT_CHECKCAST);
	stmt->checkcast_ref = &expr->node;
	stmt->checkcast_class = &vmc;

	assert_print_stmt(str_aprintf(
		"CHECKCAST:\n"
		"  checkcast_type: [%p '%s']\n"
		"  checkcast_ref: [local int 0]\n", &vmc, vmc.name), stmt);
}

static void assert_print_expr(struct string *expected, struct expression *expr)
{
	assert_tree_print(expected, &expr->node);
	expr_put(expr);
}

void assert_printed_value_expr(struct string *expected, enum vm_type type,
			       unsigned long long value)
{
	struct expression *expr;

	expr = value_expr(type, value);
	assert_print_expr(expected, expr);
}

void test_should_print_value_expression(void)
{
	assert_printed_value_expr(str_aprintf("[value int 0x0]"), J_INT, 0);
	assert_printed_value_expr(str_aprintf("[value boolean 0x1]"), J_BOOLEAN, 1);
}

void assert_printed_fvalue_expr(struct string *expected, enum vm_type type,
			        double fvalue)
{
	struct expression *expr;

	expr = fvalue_expr(type, fvalue);
	assert_print_expr(expected, expr);
}

void test_should_print_fvalue_expression(void)
{
	assert_printed_fvalue_expr(str_aprintf("[fvalue float 0.000000]"), J_FLOAT, 0.0);
	assert_printed_fvalue_expr(str_aprintf("[fvalue double 1.100000]"), J_DOUBLE, 1.1);
}

void assert_printed_local_expr(struct string *expected, enum vm_type type,
			       unsigned long local_index)
{
	struct expression *expr;

	expr = local_expr(type, local_index);
	assert_print_expr(expected, expr);
}

void test_should_print_local_expression(void)
{
	assert_printed_local_expr(str_aprintf("[local int 0]"), J_INT, 0);
	assert_printed_local_expr(str_aprintf("[local boolean 1]"), J_BOOLEAN, 1);
}

void assert_printed_temporary_expr(struct string *expected, enum vm_type type, struct var_info *var, struct var_info *var2)
{
	struct expression *expr;

	if (vm_type_is_float(type))
		expr = alloc_expression(EXPR_FLOAT_TEMPORARY, type);
	else
		expr = alloc_expression(EXPR_TEMPORARY, type);

	expr->tmp_low = var2;
#ifdef CONFIG_32_BIT
	expr->tmp_high = var;
#endif

	assert_print_expr(expected, expr);
}

void test_should_print_temporary_expression(void)
{
	assert_printed_temporary_expr(str_aprintf(
		"[temporary int 0x12345678 (low)]"),
		J_INT, NULL, (struct var_info *)0x12345678);
#ifdef CONFIG_32_BIT
	assert_printed_temporary_expr(str_aprintf(
		"[temporary boolean 0x85215975 (high), 0x87654321 (low)]"),
		J_BOOLEAN,
		(struct var_info *)0x85215975, (struct var_info *)0x87654321);
#else
	assert_printed_temporary_expr(str_aprintf(
		"[temporary boolean 0x87654321 (low)]"),
		J_BOOLEAN,
		(struct var_info *)0x85215975, (struct var_info *)0x87654321);
#endif
}

void assert_printed_array_deref_expr(struct string *expected, enum vm_type type,
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
	assert_printed_array_deref_expr(str_aprintf(
		"ARRAY_DEREF:\n"
		"  vm_type: [float]\n"
		"  arrayref: [value reference 0x0]\n"
		"  array_index: [value int 0x1]\n"), J_FLOAT, 0, 1);
	assert_printed_array_deref_expr(str_aprintf(
		"ARRAY_DEREF:\n"
		"  vm_type: [double]\n"
		"  arrayref: [value reference 0x1]\n"
		"  array_index: [value int 0x2]\n"), J_DOUBLE, 1, 2);
}

void assert_printed_binop_expr(struct string *expected, enum vm_type type,
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
	assert_printed_binop_expr(str_aprintf(
		"BINOP:\n"
		"  vm_type: [int]\n"
		"  binary_operator: [add]\n"
		"  binary_left: [value int 0x0]\n"
		"  binary_right: [value int 0x1]\n"),
		J_INT, OP_ADD, value_expr(J_INT, 0), value_expr(J_INT, 1));

	assert_printed_binop_expr(str_aprintf(
		"BINOP:\n"
		"  vm_type: [long]\n"
		"  binary_operator: [add]\n"
		"  binary_left: [value long 0x1]\n"
		"  binary_right:\n"
		"    BINOP:\n"
		"      vm_type: [long]\n"
		"      binary_operator: [sub]\n"
		"      binary_left: [value long 0x2]\n"
		"      binary_right: [value long 0x3]\n"),
		J_LONG, OP_ADD,
		value_expr(J_LONG, 1), binop_expr(J_LONG, OP_SUB,
		value_expr(J_LONG, 2), value_expr(J_LONG, 3)));
}

void assert_printed_unary_op_expr(struct string *expected, enum vm_type type,
				  enum unary_operator op,
				  struct expression *unary_expr)
{
	struct expression *expr;

	expr = unary_op_expr(type, op, unary_expr);
	assert_print_expr(expected, expr);
}

void test_should_print_unary_op_expression(void)
{
	assert_printed_unary_op_expr(str_aprintf(
		"UNARY_OP:\n"
		"  vm_type: [int]\n"
		"  unary_operator: [neg]\n"
		"  unary_expression: [value int 0x0]\n"),
		J_INT, OP_NEG, value_expr(J_INT, 0));

	assert_printed_unary_op_expr(str_aprintf(
		"UNARY_OP:\n"
		"  vm_type: [boolean]\n"
		"  unary_operator: [neg]\n"
		"  unary_expression: [value boolean 0x1]\n"),
		J_BOOLEAN, OP_NEG, value_expr(J_BOOLEAN, 1));
}

void assert_printed_conversion_expr(struct string *expected, enum vm_type type,
				    struct expression *from_expr)
{
	struct expression *expr;

	expr = conversion_expr(type, from_expr);
	assert_print_expr(expected, expr);
}

void test_should_print_conversion_expression(void)
{
	assert_printed_conversion_expr(str_aprintf(
		"CONVERSION:\n"
		"  vm_type: [long]\n"
		"  from_expression: [value int 0x0]\n"),
		J_LONG, value_expr(J_INT, 0));

	assert_printed_conversion_expr(str_aprintf(
		"CONVERSION:\n"
		"  vm_type: [int]\n"
		"  from_expression: [value boolean 0x1]\n"),
		J_INT, value_expr(J_BOOLEAN, 1));
}

void assert_printed_class_field_expr(struct string *expected, enum vm_type type,
				     struct vm_field *field)
{
	struct expression *expr;

	expr = class_field_expr(type, field);
	assert_print_expr(expected, expr);
}

void test_should_print_class_field_expression(void)
{
	assert_printed_class_field_expr(str_aprintf(
		"[class_field int %p '%s.%s']",
		&vmf, vmf.class->name, vmf.name, "I"), J_INT, &vmf);
}

void assert_printed_instance_field_expr(struct string *expected, enum vm_type type,
					struct vm_field *field,
					struct expression *objectref)
{
	struct expression *expr;

	expr = instance_field_expr(type, field, objectref);
	assert_print_expr(expected, expr);
}

void test_should_print_instance_field_expression(void)
{
	struct expression *objectref;

	objectref = value_expr(J_REFERENCE, 0xdeadbeef);

	assert_printed_instance_field_expr(str_aprintf(
		"INSTANCE_FIELD:\n"
		"  vm_type: [int]\n"
		"  instance_field: [%p '%s.%s']\n"
		"  objectref_expression: [value reference 0xdeadbeef]\n",
		&vmf, vmf.class->name, vmf.name), J_INT, &vmf, objectref);
}

void assert_printed_invoke_stmt(enum vm_type type,
				struct string *expected,
				struct vm_method *method,
				struct expression *args_list,
				struct expression *result)
{
	struct statement *stmt;

	stmt = alloc_statement(type);
	stmt->invoke_result = result;
	stmt->args_list = &args_list->node;
	stmt->target_method = method;
	assert_print_stmt(expected, stmt);
}

void test_should_print_invoke_statement(void)
{
	assert_printed_invoke_stmt(STMT_INVOKE, str_aprintf(
		"INVOKE:\n"
		"  target_method: [%p '%s.%s%s' (%lu)]\n"
		"  args_list: [no args]\n"
		"  result: [void]\n",
		&vmm, vmm.class->name, vmm.name, vmm.type, vmm.virtual_index),
		&vmm, no_args_expr(), NULL);
}

void test_should_print_invokevirtual_expression(void)
{
	assert_printed_invoke_stmt(STMT_INVOKEVIRTUAL, str_aprintf(
		"INVOKEVIRTUAL:\n"
		"  target_method: [%p '%s.%s%s' (%lu)]\n"
		"  args_list: [no args]\n"
		"  result: [void]\n",
		&vmm, vmm.class->name, vmm.name, vmm.type, vmm.virtual_index),
		&vmm, no_args_expr(), NULL);
}

void assert_printed_args_list_expr(struct string *expected,
				   struct expression *args_left,
				   struct expression *args_right)
{
	struct expression *expr;

	expr = args_list_expr(arg_expr(args_left), arg_expr(args_right));
	assert_print_expr(expected, expr);
}

void test_should_print_args_list_expression(void)
{
	assert_printed_args_list_expr(str_aprintf(
		"ARGS_LIST:\n"
		"  args_left:\n"
		"    ARG:\n"
		"      arg_expression: [value int 0x0]\n"
		"  args_right:\n"
		"    ARG:\n"
		"      arg_expression: [value boolean 0x1]\n"),
		value_expr(J_INT, 0), value_expr(J_BOOLEAN, 1));
}

void assert_printed_arg_expr(struct string *expected,
			     struct expression *arg_expression)
{
	struct expression *expr;

	expr = arg_expr(arg_expression);
	assert_print_expr(expected, expr);
}

void test_should_print_arg_expression(void)
{
	assert_printed_arg_expr(str_aprintf(
		"ARG:\n"
		"  arg_expression: [value int 0x0]\n"),
		value_expr(J_INT, 0));

	assert_printed_arg_expr(str_aprintf(
		"ARG:\n"
		"  arg_expression: [value boolean 0x1]\n"),
		value_expr(J_BOOLEAN, 1));
}

void test_should_print_no_args_expression(void)
{
	assert_print_expr(str_aprintf("[no args]"), no_args_expr());
}

void assert_printed_new_expr(struct string *expected, struct vm_class *class)
{
	struct expression *expr;

	expr = new_expr(class);
	assert_print_expr(expected, expr);
}

void test_should_print_new_expression(void)
{
	assert_printed_new_expr(str_aprintf(
		"NEW:\n"
		"  vm_type: [reference]\n"
		"  class: [%p '%s']\n", &vmc, vmc.name), &vmc);
}

void test_print_newarray_expression(void)
{
	assert_print_expr(str_aprintf(
		"NEWARRAY:\n"
		"  vm_type: [reference]\n"
		"  array_size: [value int 0xff]\n"
		"  array_type: [10]\n"),
		newarray_expr(T_INT, value_expr(J_INT, 0xff)));
}

void test_print_anewarray_expression(void)
{
	assert_print_expr(str_aprintf(
		"ANEWARRAY:\n"
		"  vm_type: [reference]\n"
		"  anewarray_size: [value int 0xff]\n"
		"  anewarray_ref_type: [%p '%s']\n",
		&vmc, vmc.name),
		anewarray_expr(&vmc, value_expr(J_INT, 0xff)));
}

void test_print_multianewarray_expression(void)
{
	struct expression *expr, *args_list;
	args_list = args_list_expr(arg_expr(value_expr(J_INT, 0x02)),
		arg_expr(value_expr(J_INT, 0xff)));

	expr = multianewarray_expr(&vmc);
	expr->multianewarray_dimensions = &args_list->node;

	assert_print_expr(str_aprintf(
		"MULTIANEWARRAY:\n"
		"  vm_type: [reference]\n"
		"  multianewarray_ref_type: [%p '%s']\n"
		"  dimension list:\n"
		"    ARGS_LIST:\n"
		"      args_left:\n"
		"        ARG:\n"
		"          arg_expression: [value int 0x2]\n"
		"      args_right:\n"
		"        ARG:\n"
		"          arg_expression: [value int 0xff]\n",
		&vmc, vmc.name), expr);
}

void test_print_arraylength_expression(void)
{
	assert_print_expr(str_aprintf(
		"ARRAYLENGTH:\n"
		"  vm_type: [int]\n"
		"  arraylength_ref: [value reference 0xcafe]\n"),
		arraylength_expr(value_expr(J_REFERENCE, 0xcafe)));
}

void test_print_instanceof_expression(void)
{
	assert_print_expr(str_aprintf(
		"INSTANCEOF:\n"
		"  vm_type: [int]\n"
		"  instanceof_class: [%p '%s']\n"
		"  instanceof_ref: [value reference 0x55]\n",
		&vmc, vmc.name),
		instanceof_expr(value_expr(J_REFERENCE, 0x55), &vmc));
}

void test_should_print_nullcheck_expression(void)
{
	struct expression *expression;
	struct expression *expr;

	expression = local_expr(J_REFERENCE, 0);

	expr = null_check_expr(expression);

	assert_print_expr(str_aprintf(
		"NULL_CHECK:\n"
		"  ref: [local reference 0]\n"), expr);
}
