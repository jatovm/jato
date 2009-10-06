/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include "jit/basic-block.h"
#include <bc-test-utils.h>
#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "jit/statement.h"
#include "lib/stack.h"
#include "lib/string.h"
#include "vm/method.h"
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
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);
	return_value = temporary_expr(vm_type, bb->b_parent);
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

	assert_int_equals(0, parse_method_type(&target_method));

	bb = __alloc_simple_bb(&method);

	objectref_expr = value_expr(J_REFERENCE, objectref);
	stack_push(bb->mimic_stack, objectref_expr);
	if (args)
		push_args(bb, args, nr_args-1);

	convert_ir_invoke(bb->b_parent, &target_method, method_index);

	return bb;
}

static void assert_invoke_passes_objectref(unsigned char invoke_opc,
					   bool nullcheck)
{
	struct statement *invoke_stmt;
	struct expression *arg_expr;
	struct basic_block *bb;

	bb = build_invoke_bb(invoke_opc, "()V", 1, 0xdeadbeef, 0, 0, NULL);

	invoke_stmt = first_stmt(bb->b_parent);
	arg_expr = to_expr(invoke_stmt->args_list);

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
	struct statement *invoke_stmt;
	struct expression *second_arg;
	struct basic_block *bb;
	struct string *str;

	str = alloc_str();

	str_append(str, "(");
	for (unsigned int i = 0; i < nr_args; i++)
		str_append(str, "I");
	str_append(str, ")V");

	create_args(args, ARRAY_SIZE(args));
	bb = build_invoke_bb(invoke_opc, str->value, nr_args+1, 0, 0, 0, args);
	free_str(str);

	invoke_stmt = first_stmt(bb->b_parent);
	args_list_expr = to_expr(invoke_stmt->args_list);
	second_arg = to_expr(args_list_expr->args_left);

	assert_args(args, ARRAY_SIZE(args), second_arg);

	__free_simple_bb(bb);
}

static void assert_invoke_return_type(unsigned char invoke_opc, enum vm_type expected, char *type)
{
	struct statement *invoke_stmt;
	struct basic_block *bb;

	bb = build_invoke_bb(invoke_opc, type, 1, 0, 0, 0, NULL);
	invoke_stmt = first_stmt(bb->b_parent);

	if (expected == J_VOID)
		assert_ptr_equals(NULL, invoke_stmt->invoke_result);
	else
		assert_temporary_expr(expected, &invoke_stmt->invoke_result->node);

	__free_simple_bb(bb);
}

static void assert_converts_to_invoke_stmt(enum vm_type expected_result_type, unsigned char opc, char *return_type, int nr_args)
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
		opc, 0x00, 0x00
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
	assert_int_equals(0, parse_method_type(&target_method));
	create_args(args, nr_args);
	push_args(bb, args, nr_args);
	convert_ir_invoke(bb->b_parent, &target_method, 0);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_invoke_stmt(expected_result_type, &target_method, &stmt->node);

	actual_args = to_expr(stmt->args_list);
	assert_args(args, nr_args, actual_args);

	if (expected_result_type == J_VOID)
		assert_true(stack_is_empty(bb->mimic_stack));
	else {
		struct expression *result;

		assert_false(stack_is_empty(bb->mimic_stack));
		result = stack_pop(bb->mimic_stack);
		assert_temporary_expr(expected_result_type, &result->node);
		expr_put(result);
		assert_true(stack_is_empty(bb->mimic_stack));
	}

	__free_simple_bb(bb);
}

/*
 * 	INVOKESPECIAL
 */

void test_invokespecial_should_pass_objectref_as_first_argument(void)
{
	assert_invoke_passes_objectref(OPC_INVOKESPECIAL, true);
}

void test_invokespecial_converts_to_invoke_stmt(void)
{
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESPECIAL, "()I", 0);
}

void test_invokespecial_should_parse_passed_arguments(void)
{
	assert_invoke_args(OPC_INVOKESPECIAL, 1);
	assert_invoke_args(OPC_INVOKESPECIAL, 2);
	assert_invoke_args(OPC_INVOKESPECIAL, 3);
}

void test_invokespecial_should_parse_return_type(void)
{
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_VOID, "()V");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_INT, "()B");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_INT, "()Z");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_INT, "()C");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_INT, "()S");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_INT, "()I");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_LONG, "()J");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_FLOAT, "()F");
	assert_invoke_return_type(OPC_INVOKESPECIAL, J_DOUBLE, "()D");
}

/*
 * 	INVOKEVIRTUAL
 */

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
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_VOID, "()V");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_INT, "()B");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_INT, "()Z");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_INT, "()C");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_INT, "()S");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_INT, "()I");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_LONG, "()J");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_FLOAT, "()F");
	assert_invoke_return_type(OPC_INVOKEVIRTUAL, J_DOUBLE, "()D");
}

/*
 * 	INVOKESTATIC
 */

void test_convert_invokestatic(void)
{
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "()B", 0);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "()Z", 0);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "()C", 0);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "()S", 0);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "()I", 0);
	assert_converts_to_invoke_stmt(J_LONG, OPC_INVOKESTATIC, "()J", 0);
	assert_converts_to_invoke_stmt(J_VOID, OPC_INVOKESTATIC, "()V", 0);
	assert_converts_to_invoke_stmt(J_FLOAT, OPC_INVOKESTATIC, "()F", 0);
	assert_converts_to_invoke_stmt(J_DOUBLE, OPC_INVOKESTATIC, "()D", 0);

	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "(I)I", 1);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "(II)I", 2);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "(III)I", 3);
	assert_converts_to_invoke_stmt(J_INT, OPC_INVOKESTATIC, "(IIIII)I", 5);
}

/* MISSING: invokeinterface */
