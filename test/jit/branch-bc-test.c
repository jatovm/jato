/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <basic-block.h>
#include <jit/compilation-unit.h>
#include <jit-compiler.h>
#include <jit/bytecode-converters.h>
#include <jit/expression.h>
#include <jit/statement.h>
#include <vm/list.h>
#include <vm/vm.h>

#include <bc-test-utils.h>
#include <libharness.h>

#define TARGET_OFFSET 2

static void assert_convert_if(enum binary_operator expected_operator,
			      unsigned char opc)
{
	struct expression *if_value;
	struct basic_block *stmt_bb, *true_bb;
	struct statement *if_stmt;
	struct compilation_unit *cu;
	unsigned char code[] = { opc, 0, TARGET_OFFSET };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};

	stmt_bb = alloc_basic_block(0, 1);
	true_bb = alloc_basic_block(TARGET_OFFSET, TARGET_OFFSET + 1);

	cu = alloc_compilation_unit(&method);
	list_add_tail(&stmt_bb->bb_list_node, &cu->bb_list);
	list_add_tail(&true_bb->bb_list_node, &cu->bb_list);

	if_value = temporary_expr(J_INT, 1);
	stack_push(cu->expr_stack, if_value);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	if_stmt = stmt_entry(stmt_bb->stmt_list.next);
	assert_int_equals(STMT_IF, stmt_type(if_stmt));
	assert_ptr_equals(true_bb->label_stmt, if_stmt->if_true);
	__assert_binop_expr(J_INT, expected_operator, if_stmt->if_conditional);
	assert_ptr_equals(if_value, to_expr(to_expr(if_stmt->if_conditional)->binary_left));
	assert_value_expr(J_INT, 0, to_expr(if_stmt->if_conditional)->binary_right);

	free_compilation_unit(cu);
}

void test_convert_if(void)
{
	assert_convert_if(OP_EQ, OPC_IFEQ);
	assert_convert_if(OP_NE, OPC_IFNE);
	assert_convert_if(OP_LT, OPC_IFLT);
	assert_convert_if(OP_GE, OPC_IFGE);
	assert_convert_if(OP_GT, OPC_IFGT);
	assert_convert_if(OP_LE, OPC_IFLE);
}

static void assert_convert_if_cmp(enum binary_operator expected_operator,
				  enum jvm_type jvm_type, unsigned char opc)
{
	struct expression *if_value1, *if_value2;
	struct basic_block *stmt_bb, *true_bb;
	struct statement *if_stmt;
	struct compilation_unit *cu;
	unsigned char code[] = { opc, 0, TARGET_OFFSET };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};

	stmt_bb = alloc_basic_block(0, 1);
	true_bb = alloc_basic_block(TARGET_OFFSET, TARGET_OFFSET + 1);

	cu = alloc_compilation_unit(&method);
	list_add_tail(&stmt_bb->bb_list_node, &cu->bb_list);
	list_add_tail(&true_bb->bb_list_node, &cu->bb_list);

	if_value1 = temporary_expr(jvm_type, 1);
	stack_push(cu->expr_stack, if_value1);

	if_value2 = temporary_expr(jvm_type, 2);
	stack_push(cu->expr_stack, if_value2);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	if_stmt = stmt_entry(stmt_bb->stmt_list.next);
	assert_int_equals(STMT_IF, stmt_type(if_stmt));
	assert_ptr_equals(true_bb->label_stmt, if_stmt->if_true);
	assert_binop_expr(jvm_type, expected_operator, if_value1, if_value2,
			  if_stmt->if_conditional);

	free_compilation_unit(cu);
}

void test_convert_if_icmp(void)
{
	assert_convert_if_cmp(OP_EQ, J_INT, OPC_IF_ICMPEQ);
	assert_convert_if_cmp(OP_NE, J_INT, OPC_IF_ICMPNE);
	assert_convert_if_cmp(OP_LT, J_INT, OPC_IF_ICMPLT);
	assert_convert_if_cmp(OP_GE, J_INT, OPC_IF_ICMPGE);
	assert_convert_if_cmp(OP_GT, J_INT, OPC_IF_ICMPGT);
	assert_convert_if_cmp(OP_LE, J_INT, OPC_IF_ICMPLE);
}

void test_convert_if_acmp(void)
{
	assert_convert_if_cmp(OP_EQ, J_REFERENCE, OPC_IF_ACMPEQ);
	assert_convert_if_cmp(OP_NE, J_REFERENCE, OPC_IF_ACMPNE);
}

void test_convert_goto(void)
{
	struct basic_block *goto_bb, *target_bb;
	struct statement *goto_stmt;
	struct compilation_unit *cu;
	unsigned char code[] = { OPC_GOTO, 0, TARGET_OFFSET };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};

	goto_bb = alloc_basic_block(0, 1);
	target_bb = alloc_basic_block(TARGET_OFFSET, TARGET_OFFSET + 1);

	cu = alloc_compilation_unit(&method);
	list_add_tail(&goto_bb->bb_list_node, &cu->bb_list);
	list_add_tail(&target_bb->bb_list_node, &cu->bb_list);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	goto_stmt = stmt_entry(goto_bb->stmt_list.next);
	assert_int_equals(STMT_GOTO, stmt_type(goto_stmt));
	assert_ptr_equals(target_bb->label_stmt, goto_stmt->goto_target);

	free_compilation_unit(cu);
}

void test_convert_ifnull(void)
{
	assert_convert_if(OP_EQ, OPC_IFNULL);
}

void test_convert_ifnonnull(void)
{
	assert_convert_if(OP_NE, OPC_IFNONNULL);
}
