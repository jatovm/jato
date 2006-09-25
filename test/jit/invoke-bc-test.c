/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <jit/basic-block.h>
#include <bc-test-utils.h>
#include <jit/compilation-unit.h>
#include <jit/jit-compiler.h>
#include <jit/statement.h>
#include <vm/stack.h>

#include <libharness.h>

static void assert_convert_return(enum jvm_type jvm_type, unsigned char opc)
{
	struct expression *return_value;
	struct compilation_unit *cu;
	unsigned char code[] = { opc };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct statement *ret_stmt;

	cu = alloc_simple_compilation_unit(&method);

	return_value = temporary_expr(jvm_type, 0);
	stack_push(cu->expr_stack, return_value);

	convert_to_ir(cu);
	ret_stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	assert_true(stack_is_empty(cu->expr_stack));
	assert_return_stmt(return_value, ret_stmt);

	free_compilation_unit(cu);
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
	struct compilation_unit *cu;
	unsigned char code[] = { OPC_RETURN };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct statement *ret_stmt;

	cu = alloc_simple_compilation_unit(&method);

	convert_to_ir(cu);
	ret_stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	assert_true(stack_is_empty(cu->expr_stack));
	assert_void_return_stmt(ret_stmt);

	free_compilation_unit(cu);
}

static void create_args(struct expression **args, int nr_args)
{
	int i;

	for (i = 0; i < nr_args; i++) {
		args[i] = value_expr(J_INT, i);
	}
}

static void push_args(struct compilation_unit *cu,
		      struct expression **args, int nr_args)
{
	int i;

	for (i = 0; i < nr_args; i++) {
		stack_push(cu->expr_stack, args[i]);
	}
}

static void assert_args(struct expression **expected_args,
			int nr_args,
			struct expression *args_list)
{
	int i;
	struct expression *tree = args_list;
	struct expression *actual_args[nr_args];

	if (nr_args == 0) {
		assert_int_equals(EXPR_NO_ARGS, expr_type(args_list));
		return;
	}

	i = 0;
	while (i < nr_args) {
		if (expr_type(tree) == EXPR_ARGS_LIST) {
			struct expression *expr = to_expr(tree->node.kids[1]);
			actual_args[i++] = to_expr(expr->arg_expression);
			tree = to_expr(tree->node.kids[0]);
		} else if (expr_type(tree) == EXPR_ARG) {
			actual_args[i++] = to_expr(tree->arg_expression);
			break;
		} else {
			assert_true(false);
			break;
		}
	}

	assert_int_equals(i, nr_args);
	
	for (i = 0; i < nr_args; i++)
		assert_ptr_equals(expected_args[i], actual_args[i]);
}

static void convert_ir_invoke(struct compilation_unit *cu,
			      struct methodblock *target_method,
			      unsigned long method_index)
{
	u4 cp_infos[method_index + 1];
	u1 cp_types[method_index + 1];

	cp_infos[method_index] = (unsigned long) target_method;
	cp_types[method_index] = CONSTANT_Resolved;
	convert_ir_const(cu, (void *)cp_infos, method_index+1, cp_types);
}

static struct compilation_unit *
create_invoke_x_unit(unsigned char invoke_opc,
		     char *type, unsigned long nr_args,
		     unsigned long objectref,
		     unsigned short method_index,
		     unsigned short method_table_idx,
		     struct expression **args)
{
	struct methodblock target_method = {
		.type = type,
		.args_count = nr_args,
		.method_table_index = method_table_idx,
	};
	unsigned char code[] = {
		invoke_opc, (method_index >> 8) & 0xff, method_index & 0xff,
	};
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct expression *objectref_expr;
	struct compilation_unit *cu;

	cu = alloc_simple_compilation_unit(&method);

	objectref_expr = value_expr(J_REFERENCE, objectref);
	stack_push(cu->expr_stack, objectref_expr);
	if (args)
		push_args(cu, args, nr_args-1);

	convert_ir_invoke(cu, &target_method, method_index);
	return cu;
}

static void assert_invoke_expression_type(enum expression_type expected_type, unsigned char invoke_opc)
{
	struct expression *invoke_expr;
	struct compilation_unit *cu;
	struct statement *stmt;

	cu = create_invoke_x_unit(invoke_opc, "()V", 1, 0, 0, 0, NULL);

	stmt = first_stmt(cu);
	invoke_expr = to_expr(stmt->expression);

	assert_int_equals(expected_type, expr_type(invoke_expr));

	free_compilation_unit(cu);
}

static void assert_invoke_method_idx(unsigned long expected_idx, unsigned char invoke_opc)
{
	struct compilation_unit *cu;
	struct statement *stmt;
	struct expression *invoke_expr;

	cu = create_invoke_x_unit(invoke_opc, "()V", 1, 0, 0xcafe, expected_idx, NULL);

	stmt = first_stmt(cu);
	assert_not_null(stmt->expression);
	invoke_expr = to_expr(stmt->expression);

	assert_int_equals(expected_idx, invoke_expr->method_index);

	free_compilation_unit(cu);
}

static void assert_invoke_passes_objectref(unsigned char invoke_opc)
{
	struct compilation_unit *cu;
	struct statement *stmt;
	struct expression *invoke_expr;
	struct expression *arg_expr;

	cu = create_invoke_x_unit(invoke_opc, "()V", 1, 0xdeadbeef, 0, 0, NULL);

	stmt = first_stmt(cu);
	invoke_expr = to_expr(stmt->expression);
	arg_expr = to_expr(invoke_expr->args_list);

	assert_value_expr(J_REFERENCE, 0xdeadbeef, arg_expr->arg_expression);

	free_compilation_unit(cu);
}

static void assert_invoke_args(unsigned char invoke_opc, unsigned long nr_args)
{
	struct compilation_unit *cu;
	struct statement *stmt;
	struct expression *invoke_expr;
	struct expression *args[nr_args];
	struct expression *args_list_expr;
	struct expression *second_arg;

	create_args(args, ARRAY_SIZE(args));
	cu = create_invoke_x_unit(invoke_opc, "()V", nr_args+1, 0, 0, 0, args);

	stmt = first_stmt(cu);
	invoke_expr = to_expr(stmt->expression);
	args_list_expr = to_expr(invoke_expr->args_list);
	second_arg = to_expr(args_list_expr->args_left);

	assert_args(args, ARRAY_SIZE(args), second_arg);

	free_compilation_unit(cu);
}

static void assert_invoke_return_type(unsigned char invoke_opc, enum jvm_type expected, char *type)
{
	struct compilation_unit *cu;
	struct expression *invoke_expr;

	cu = create_invoke_x_unit(invoke_opc, type, 1, 0, 0, 0, NULL);
	invoke_expr = stack_pop(cu->expr_stack);
	assert_int_equals(expected, invoke_expr->jvm_type);

	expr_put(invoke_expr);
	free_compilation_unit(cu);
}

static struct compilation_unit *invoke_discarded_return_value(unsigned char invoke_opc,
							      struct methodblock *target_method)
{
	struct compilation_unit *cu;
	unsigned char code[] = {
		invoke_opc, 0x00, 0x00,
		OPC_POP
	};
	struct methodblock invoker_method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	
	target_method->type = "()I";
	target_method->args_count = 0;

	cu = alloc_simple_compilation_unit(&invoker_method);
	convert_ir_invoke(cu, target_method, 0);

	return cu;
}

static void assert_invoke_return_value_discarded(enum expression_type expected_type, unsigned char invoke_opc)
{
	struct methodblock target_method;
	struct compilation_unit *cu;
	struct statement *stmt;

	cu = invoke_discarded_return_value(invoke_opc, &target_method);
	stmt = first_stmt(cu);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_int_equals(expected_type, expr_type(to_expr(stmt->expression)));
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

/*
 * 	INVOKESPECIAL
 */

void test_invokespecial_should_be_converted_to_invokespecial_expr(void)
{
	assert_invoke_expression_type(EXPR_INVOKESPECIAL, OPC_INVOKESPECIAL);
}

void test_invokespecial_should_parse_method_index_for_expr(void)
{
	assert_invoke_method_idx(0xcafe, OPC_INVOKESPECIAL);
}

void test_invokespecial_should_pass_objectref_as_first_argument(void)
{
	assert_invoke_passes_objectref(OPC_INVOKESPECIAL);
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
	assert_invoke_return_value_discarded(EXPR_INVOKESPECIAL, OPC_INVOKESPECIAL);
}

/*
 * 	INVOKEVIRTUAL
 */

void test_invokevirtual_should_be_converted_to_invokevirtual_expr(void)
{
	assert_invoke_expression_type(EXPR_INVOKEVIRTUAL, OPC_INVOKEVIRTUAL);
}

void test_invokevirtual_should_parse_method_index_for_expr(void)
{
	assert_invoke_method_idx(0xbabe, OPC_INVOKEVIRTUAL);
}

void test_invokevirtual_should_pass_objectref_as_first_argument(void)
{
	assert_invoke_passes_objectref(OPC_INVOKEVIRTUAL);
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

static void assert_convert_invokestatic(enum jvm_type expected_jvm_type,
					char *return_type, int nr_args)
{
	struct methodblock target_method = {
		.type = return_type,
		.args_count = nr_args,
	};
	unsigned char code[] = {
		OPC_INVOKESTATIC, 0x00, 0x00,
		OPC_IRETURN
	};
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct statement *stmt;
	struct expression *args[nr_args];
	struct expression *actual_args;

	cu = alloc_simple_compilation_unit(&method);

	create_args(args, nr_args);
	push_args(cu, args, nr_args);
	convert_ir_invoke(cu, &target_method, 0);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_int_equals(STMT_RETURN, stmt_type(stmt));
	assert_invoke_expr(expected_jvm_type, &target_method, stmt->return_value);

	actual_args = to_expr(to_expr(stmt->return_value)->args_list);
	assert_args(args, nr_args, actual_args);

	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_invokestatic(void)
{
	assert_convert_invokestatic(J_BYTE, "()B", 0);
	assert_convert_invokestatic(J_INT, "()I", 0);
	assert_convert_invokestatic(J_INT, "()I", 1);
	assert_convert_invokestatic(J_INT, "()I", 2);
	assert_convert_invokestatic(J_INT, "()I", 3);
	assert_convert_invokestatic(J_INT, "()I", 5);
}

void test_convert_invokestatic_for_void_return_type(void)
{
	struct methodblock mb;
	unsigned char code[] = {
		OPC_INVOKESTATIC, 0x00, 0x00,
	};
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct statement *stmt;

	mb.type = "()V";
	mb.args_count = 0;

	cu = alloc_simple_compilation_unit(&method);
	convert_ir_invoke(cu, &mb, 0);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_invoke_expr(J_VOID, &mb, stmt->expression);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_invokestatic_when_return_value_is_discarded(void)
{
	struct compilation_unit *cu;
	struct statement *stmt;
	struct methodblock mb;

	cu = invoke_discarded_return_value(OPC_INVOKESTATIC, &mb);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_invoke_expr(J_INT, &mb, stmt->expression);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

/* MISSING: invokeinterface */
