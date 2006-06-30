/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <basic-block.h>
#include <bc-test-utils.h>
#include <compilation-unit.h>
#include <jit/statement.h>
#include <vm/stack.h>

#include <libharness.h>

/* MISSING: invokevirtual */

/* MISSING: invokespecial */

static void push_args(struct compilation_unit *cu,
		      struct expression **args, int nr_args)
{
	int i;

	for (i = 0; i < nr_args; i++) {
		args[i] = value_expr(J_INT, i);
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

	i = 0;
	while (i < nr_args) {
		if (expr_type(tree) == EXPR_ARGS_LIST) {
			struct expression *expr = to_expr(tree->node.kids[1]);
			actual_args[i++] = to_expr(expr->arg_expression);
			tree = to_expr(tree->node.kids[0]);
		} else if (expr_type(tree) == EXPR_ARG) {
			actual_args[i++] = to_expr(tree->arg_expression);
			break;
		} else
			assert_true(false);
	}

	assert_int_equals(i, nr_args);
	
	for (i = 0; i < nr_args; i++)
		assert_ptr_equals(expected_args[i], actual_args[i]);
}

static void convert_ir_invoke(struct compilation_unit *cu, struct methodblock *mb)
{
	u8 cp_infos[] = { (unsigned long) mb };
	u1 cp_types[] = { CONSTANT_Resolved };

	convert_ir_const(cu, (void *)cp_infos, 8, cp_types);
}

static void assert_convert_invokestatic(enum jvm_type expected_jvm_type,
					char *return_type, int nr_args)
{
	struct methodblock mb;
	unsigned char code[] = {
		OPC_INVOKESTATIC, 0x00, 0x00,
		OPC_IRETURN
	};
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct statement *stmt;
	struct expression *args[nr_args];
	struct expression *actual_args;

	mb.type = return_type;
	mb.args_count = nr_args;

	cu = alloc_simple_compilation_unit(&method);

	push_args(cu, args, nr_args);
	convert_ir_invoke(cu, &mb);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_int_equals(STMT_RETURN, stmt_type(stmt));
	assert_invoke_expr(expected_jvm_type, &mb, stmt->return_value);

	actual_args = to_expr(to_expr(stmt->return_value)->args_list);

	if (nr_args)
		assert_args(args, nr_args, actual_args);
	else
		assert_int_equals(EXPR_NO_ARGS, expr_type(actual_args));

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
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct statement *stmt;

	mb.type = "()V";
	mb.args_count = 0;

	cu = alloc_simple_compilation_unit(&method);
	convert_ir_invoke(cu, &mb);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_invoke_expr(J_VOID, &mb, stmt->expression);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_invokestatic_when_return_value_is_discarded(void)
{
	struct methodblock mb;
	unsigned char code[] = {
		OPC_INVOKESTATIC, 0x00, 0x00,
		OPC_POP
	};
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct statement *stmt;

	mb.type = "()I";
	mb.args_count = 0;

	cu = alloc_simple_compilation_unit(&method);
	convert_ir_invoke(cu, &mb);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_int_equals(STMT_EXPRESSION, stmt_type(stmt));
	assert_invoke_expr(J_INT, &mb, stmt->expression);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

/* MISSING: invokeinterface */
