/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <jit/basic-block.h>
#include <jit/compilation-unit.h>
#include <jit/compiler.h>
#include <jit/bytecode-converters.h>
#include <jit/expression.h>
#include <jit/statement.h>
#include <vm/list.h>
#include <vm/vm.h>

#include <bc-test-utils.h>
#include <libharness.h>

/*
 * The branch bytes contain a relative branch target offset from the beginning
 * of the branch bytecode instruction.
 */
#define BRANCH_INSN_SIZE 3

/* Where we are branching at.  */
#define BRANCH_OFFSET 0

/* Target offset stored in the branch instruction  */
#define BRANCH_TARGET 4

/* The absolute offset we are branching to  */
#define TARGET_OFFSET BRANCH_OFFSET + BRANCH_TARGET

static void assert_convert_if(enum binary_operator expected_operator,
			      unsigned char opc)
{
	struct basic_block *branch_bb, *bb, *target_bb;
	struct statement *if_stmt;
	struct expression *if_value;
	struct compilation_unit *cu;
	unsigned char code[] = { opc, 0, BRANCH_TARGET, OPC_NOP, OPC_NOP };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_compilation_unit(&method);

	branch_bb = alloc_basic_block(cu, 0, BRANCH_OFFSET + BRANCH_INSN_SIZE);
	bb        = alloc_basic_block(cu, BRANCH_OFFSET + BRANCH_INSN_SIZE, TARGET_OFFSET);
	target_bb = alloc_basic_block(cu, TARGET_OFFSET, TARGET_OFFSET + 1);

	list_add_tail(&branch_bb->bb_list_node, &cu->bb_list);
	list_add_tail(&bb->bb_list_node, &cu->bb_list);
	list_add_tail(&target_bb->bb_list_node, &cu->bb_list);

	if_value = temporary_expr(J_INT);
	stack_push(cu->expr_stack, if_value);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	if_stmt = stmt_entry(branch_bb->stmt_list.next);
	assert_int_equals(STMT_IF, stmt_type(if_stmt));
	assert_ptr_equals(target_bb, if_stmt->if_true);
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
				  enum vm_type vm_type, unsigned char opc)
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

	cu = alloc_compilation_unit(&method);
	stmt_bb = alloc_basic_block(cu, 0, 1);
	true_bb = alloc_basic_block(cu, TARGET_OFFSET, TARGET_OFFSET + 1);

	list_add_tail(&stmt_bb->bb_list_node, &cu->bb_list);
	list_add_tail(&true_bb->bb_list_node, &cu->bb_list);

	if_value1 = temporary_expr(vm_type);
	stack_push(cu->expr_stack, if_value1);

	if_value2 = temporary_expr(vm_type);
	stack_push(cu->expr_stack, if_value2);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	if_stmt = stmt_entry(stmt_bb->stmt_list.next);
	assert_int_equals(STMT_IF, stmt_type(if_stmt));
	assert_ptr_equals(true_bb, if_stmt->if_true);
	assert_binop_expr(vm_type, expected_operator, if_value1, if_value2,
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

	cu = alloc_compilation_unit(&method);
	goto_bb = alloc_basic_block(cu, 0, 1);
	target_bb = alloc_basic_block(cu, TARGET_OFFSET, TARGET_OFFSET + 1);

	list_add_tail(&goto_bb->bb_list_node, &cu->bb_list);
	list_add_tail(&target_bb->bb_list_node, &cu->bb_list);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	goto_stmt = stmt_entry(goto_bb->stmt_list.next);
	assert_int_equals(STMT_GOTO, stmt_type(goto_stmt));
	assert_ptr_equals(target_bb, goto_stmt->goto_target);

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

static unsigned char is_zero_bytecode[] = {
	OPC_ILOAD_0,
	OPC_ICONST_0,
	OPC_IF_ICMPEQ, 0x00, 0x05,

	OPC_ICONST_0,
	OPC_IRETURN,

	OPC_ICONST_1,
	OPC_IRETURN,
};

void test_insn_after_branch_are_added_to_another_bb(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;
	struct methodblock method = {
		.jit_code = is_zero_bytecode,
		.code_size = ARRAY_SIZE(is_zero_bytecode),
	};

	cu = alloc_compilation_unit(&method);

	analyze_control_flow(cu);
	convert_to_ir(cu);

	bb = list_first_entry(&cu->bb_list, struct basic_block, bb_list_node);
	assert_false(list_is_empty(&bb->stmt_list));

	bb = list_next_entry(&bb->bb_list_node, struct basic_block, bb_list_node);
	assert_false(list_is_empty(&bb->stmt_list));

	free_compilation_unit(cu);
}
