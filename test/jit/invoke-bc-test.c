/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include "jit/basic-block.h"
#include <bc-test-utils.h>
#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "jit/statement.h"
#include "vm/stack.h"
#include <args-test-utils.h>

#include <libharness.h>

static void assert_convert_return(enum vm_type vm_type, unsigned char opc)
{
	struct expression *return_value;
	unsigned char code[] = { opc };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct statement *ret_stmt;
	struct var_info *temporary;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);
	temporary = get_var(bb->b_parent, J_INT);
	return_value = temporary_expr(vm_type, NULL, temporary);
	stack_push(bb->mimic_stack, return_value);
	convert_to_ir(bb->b_parent);

	ret_stmt = stmt_entry(bb->stmt_list.next);
	assert_true(stack_is_empty(bb->mimic_stack));
	assert_return_stmt(return_value, ret_stmt);

	__free_simple_bb(bb);
}

void test_convert_non_void_return(void)
{
	assert_convert_return(J_INT, OPC_IRETURN);
	assert_convert_return(J_LONG, OPC_LRETURN);
	assert_convert_return(J_FLOAT, OPC_FRETURN);
	assert_convert_return(J_DOUBLE, OPC_DRETURN);
	assert_convert_return(J_REFERENCE, OPC_ARETURN);
}

void test_convert_void_return(void)
{
	unsigned char code[] = { OPC_RETURN };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct statement *ret_stmt;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);
	convert_to_ir(bb->b_parent);

	ret_stmt = stmt_entry(bb->stmt_list.next);
	assert_true(stack_is_empty(bb->mimic_stack));
	assert_void_return_stmt(ret_stmt);

	__free_simple_bb(bb);
}

static void convert_ir_invoke(struct compilation_unit *cu,
			      struct vm_method *target_method,
			      unsigned long method_index)
{
	assert(method_index == 0);

	struct vm_class target_vmc = {
		.state = VM_CLASS_INITIALIZED,
	};

	struct vm_class vmc = {
		.methods = target_method,
	};

	target_method->class = &target_vmc;
	cu->method->class = &vmc;

	convert_to_ir(cu);
}

static struct basic_block *
build_invoke_bb(unsigned char invoke_opc,
		char *type, unsigned long nr_args,
		unsigned long objectref,
		unsigned short method_index,
		unsigned short method_table_idx,
		struct expression **args)
{
	const struct cafebabe_method_info target_method_info = {
		.access_flags = 0,
	};

	struct vm_method target_method = {
		.method = &target_method_info,
		.type = type,
		.args_count = nr_args,
		.method_index = method_table_idx,
	};
	unsigned char code[] = {
		invoke_opc, (method_index >> 8) & 0xff, method_index & 0xff,
	};
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *objectref_expr;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);

	objectref_expr = value_expr(J_REFERENCE, objectref);
	stack_push(bb->mimic_stack, objectref_expr);
	if (args)
		push_args(bb, args, nr_args-1);

	convert_ir_invoke(bb->b_parent, &target_method, method_index);

	return bb;
}

static void assert_invoke_expression_type(enum expression_type expected_type, unsigned char invoke_opc)
{
	struct expression *invoke_expr;
	struct basic_block *bb;
	struct statement *stmt;

	bb = build_invoke_bb(invoke_opc, "()V", 1, 0, 0, 0, NULL);
	stmt = first_stmt(bb->b_parent);
	invoke_expr = to_expr(stmt->expression);

	assert_int_equals(expected_type, expr_type(invoke_expr));

	__free_simple_bb(bb);
}

static void assert_invoke_passes_objectref(unsigned char invoke_opc,
					   bool nullcheck)
{
	struct expression *invoke_expr;
	struct expression *arg_expr;
	struct basic_block *bb;
	struct statement *stmt;

	bb = build_invoke_bb(invoke_opc, "()V", 1, 0xdeadbeef, 0, 0, NULL);

	stmt = first_stmt(bb->b_parent);
	invoke_expr = to_expr(stmt->expression);
	arg_expr = to_expr(invoke_expr->args_list);

	if (nullcheck)
		assert_nullcheck_value_expr(J_REFERENCE, 0xdeadbeef,
					    arg_expr->arg_expression);
	else
		assert_value_expr(J_REFERENCE, 0xdeadbeef,
				  arg_expr->arg_expression);
	__free_simple_bb(bb);
}

static void assert_invoke_args(unsigned char invoke_opc, unsigned long nr_args)
{
	struct expression *args_list_expr;
	struct expression *args[nr_args];
	struct expression *invoke_expr;
	struct expression *second_arg;
	struct basic_block *bb;
	struct statement *stmt;

	create_args(args, ARRAY_SIZE(args));
	bb = build_invoke_bb(invoke_opc, "()V", nr_args+1, 0, 0, 0, args);

	stmt = first_stmt(bb->b_parent);
	invoke_expr = to_expr(stmt->expression);
	args_list_expr = to_expr(invoke_expr->args_list);
	second_arg = to_expr(args_list_expr->args_left);

	assert_args(args, ARRAY_SIZE(args), second_arg);

	__free_simple_bb(bb);
}

static void assert_invoke_return_type(unsigned char invoke_opc, enum vm_type expected, char *type)
{
	struct expression *invoke_expr;
	struct basic_block *bb;

	bb = build_invoke_bb(invoke_opc, type, 1, 0, 0, 0, NULL);
	invoke_expr = stack_pop(bb->mimic_stack);
	assert_int_equals(expected, invoke_expr->vm_type);

	expr_put(invoke_expr);
	__free_simple_bb(bb);
}

static struct basic_block *
invoke_discarded_return_value(unsigned char invoke_opc, struct vm_method *target_method)
{
	const struct cafebabe_method_info target_method_info = {
		.access_flags = CAFEBABE_METHOD_ACC_STATIC,
	};

	unsigned char code[] = {
		invoke_opc, 0x00, 0x00,
		OPC_POP
	};
	struct vm_method invoker_method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;
	
	target_method->type = "()I";
	target_method->args_count = 0;
	target_method->method = &target_method_info;

	bb = __alloc_simple_bb(&invoker_method);
	convert_ir_invoke(bb->b_parent, target_method, 0);

	return bb;
}

static void assert_invoke_return_value_discarded(enum expression_type expected_type, unsigned char invoke_opc)
{
	struct vm_method target_method;
	struct basic_block *bb;
	struct statement *stmt;

	bb = invoke_discarded_return_value(invoke_opc, &target_method);
	stmt = first_stmt(bb->b_parent);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_int_equals(expected_type, expr_type(to_expr(stmt->expression)));
	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

static void assert_converts_to_invoke_expr(enum vm_type expected_vm_type, unsigned char opc, char *return_type, int nr_args)
{
	const struct cafebabe_method_info target_method_info = {
		.access_flags = CAFEBABE_METHOD_ACC_STATIC,
	};

	struct vm_method target_method = {
		.method = &target_method_info,
		.type = return_type,
		.args_count = nr_args,
	};
	unsigned char code[] = {
		opc, 0x00, 0x00,
		OPC_IRETURN
	};
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *args[nr_args];
	struct expression *actual_args;
	struct basic_block *bb;
	struct statement *stmt;

	bb = __alloc_simple_bb(&method);

	create_args(args, nr_args);
	push_args(bb, args, nr_args);
	convert_ir_invoke(bb->b_parent, &target_method, 0);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_int_equals(STMT_RETURN, stmt_type(stmt));
	assert_invoke_expr(expected_vm_type, &target_method, stmt->return_value);

	actual_args = to_expr(to_expr(stmt->return_value)->args_list);
	assert_args(args, nr_args, actual_args);

	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

/*
 * 	INVOKESPECIAL
 */

void test_invokespecial_should_be_converted_to_invokespecial_expr(void)
{
	assert_invoke_expression_type(EXPR_INVOKE, OPC_INVOKESPECIAL);
}

void test_invokespecial_should_pass_objectref_as_first_argument(void)
{
	assert_invoke_passes_objectref(OPC_INVOKESPECIAL, true);
}

void test_invokespecial_converts_to_invoke_expr(void)
{
	assert_converts_to_invoke_expr(J_INT, OPC_INVOKESPECIAL, "()I", 0);
}

void test_invokespecial_should_parse_passed_arguments(void)
{
	assert_invoke_args(OPC_INVOKESPECIAL, 1);
	assert_invoke_args(OPC_INVOKESPECIAL, 2);
	assert_invoke_args(OPC_INVOKESPECIAL, 3);
}

void test_invokespecial_should_parse_return_type(void)
{
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_BYTE, "()B");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_INT, "()I");
}

void test_convert_invokespecial_when_return_value_is_discarded(void)
{
	assert_invoke_return_value_discarded(EXPR_INVOKE, OPC_INVOKESPECIAL);
}

/*
 * 	INVOKEVIRTUAL
 */

void test_invokevirtual_should_be_converted_to_invokevirtual_expr(void)
{
	assert_invoke_expression_type(EXPR_INVOKEVIRTUAL, OPC_INVOKEVIRTUAL);
}

void test_invokevirtual_should_pass_objectref_as_first_argument(void)
{
	assert_invoke_passes_objectref(OPC_INVOKEVIRTUAL, false);
}

void test_invokevirtual_should_parse_passed_arguments(void)
{
	assert_invoke_args(OPC_INVOKEVIRTUAL, 1);
	assert_invoke_args(OPC_INVOKEVIRTUAL, 2);
	assert_invoke_args(OPC_INVOKEVIRTUAL, 3);
}

void test_invokevirtual_should_parse_return_type(void)
{
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_BYTE, "()B");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_INT, "()I");
}

void test_convert_invokevirtual_when_return_value_is_discarded(void)
{
	assert_invoke_return_value_discarded(EXPR_INVOKEVIRTUAL, OPC_INVOKEVIRTUAL);
}

/*
 * 	INVOKESTATIC
 */

void test_convert_invokestatic(void)
{
	assert_converts_to_invoke_expr(J_BYTE, OPC_INVOKESTATIC, "()B", 0);
	assert_converts_to_invoke_expr(J_INT, OPC_INVOKESTATIC, "()I", 0);
	assert_converts_to_invoke_expr(J_INT, OPC_INVOKESTATIC, "()I", 1);
	assert_converts_to_invoke_expr(J_INT, OPC_INVOKESTATIC, "()I", 2);
	assert_converts_to_invoke_expr(J_INT, OPC_INVOKESTATIC, "()I", 3);
	assert_converts_to_invoke_expr(J_INT, OPC_INVOKESTATIC, "()I", 5);
}

void test_convert_invokestatic_for_void_return_type(void)
{
	struct vm_method mb;
	unsigned char code[] = {
		OPC_INVOKESTATIC, 0x00, 0x00,
	};

	const struct cafebabe_method_info target_method_info = {
		.access_flags = CAFEBABE_METHOD_ACC_STATIC,
	};

	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;
	struct statement *stmt;

	mb.type = "()V";
	mb.args_count = 0;
	mb.method = &target_method_info;

	bb = __alloc_simple_bb(&method);
	convert_ir_invoke(bb->b_parent, &mb, 0);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_invoke_expr(J_VOID, &mb, stmt->expression);
	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

void test_convert_invokestatic_when_return_value_is_discarded(void)
{
	struct basic_block *bb;
	struct statement *stmt;
	struct vm_method mb;

	bb = invoke_discarded_return_value(OPC_INVOKESTATIC, &mb);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_invoke_expr(J_INT, &mb, stmt->expression);
	assert_true(stack_is_empty(bb->mimic_stack));

	free_compilation_unit(bb->b_parent);
}

/* MISSING: invokeinterface */
