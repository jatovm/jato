/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <basic-block.h>
#include <expression.h>
#include <instruction.h>
#include <insn-selector.h>
#include <statement.h>

static void assert_insn(enum insn_opcode insn_op,
			enum reg src_base_reg,
			unsigned long src_displacement,
			enum reg dest_reg,
			struct insn *insn)
{
	assert_int_equals(insn_op, insn->insn_op);
	assert_int_equals(src_base_reg, insn->src.base_reg);
	assert_int_equals(src_displacement, insn->src.disp);
	assert_int_equals(dest_reg, insn->dest.reg);
}

static void assert_insns_for_add(enum reg dest_reg,
				 unsigned long left_local,
				 unsigned long right_local,
				 unsigned long left_displacement,
				 unsigned long right_displacement)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *expr;
	struct statement *stmt;

	expr = binop_expr(J_INT, OP_ADD, local_expr(J_INT, left_local),
			  local_expr(J_INT, right_local));
	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &expr->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);
	assert_insn(MOV, REG_EBP, right_displacement, REG_EAX, insn_entry(bb->insn_list.next));
	assert_insn(ADD, REG_EBP, left_displacement, REG_EAX, insn_entry(bb->insn_list.next->next));

	free_basic_block(bb);
}

void test_should_select_nothing_for_void_return_statement(void)
{
	struct statement *stmt;
	struct basic_block *bb = alloc_basic_block(0, 1);

	stmt = alloc_statement(STMT_VOID_RETURN);
	bb_insert_stmt(bb, stmt);
	insn_select(bb);
	assert_true(list_is_empty(&bb->insn_list));
	free_basic_block(bb);
}

void test_select_insns_for_add(void)
{
	assert_insns_for_add(REG_EAX, 0, 1, 8, 12);
}

static void assert_insn_imm(enum insn_opcode expected_opc,
			    unsigned long expected_imm,
			    struct insn *insn)
{
	assert_int_equals(expected_opc, insn->insn_op);
	assert_int_equals(expected_imm, insn->operand.imm);
}

void test_select_insn_for_invoke_without_args(void)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *expr, *args_list;
	struct statement *stmt;
	struct insn *insn;

	args_list = no_args_expr();
	expr = invoke_expr(J_INT, NULL);
	expr->args_list = &args_list->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	insn = insn_entry(bb->insn_list.next);
	assert_int_equals(INSN_CALL, insn->insn_op);

	free_basic_block(bb);
}

void test_select_insn_for_invoke_with_args_list(void)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *invoke_expression, *args_list_expression;
	struct statement *stmt;

	args_list_expression = args_list_expr(
		arg_expr(value_expr(J_INT, 0x02)),
		arg_expr(value_expr(J_INT, 0x01)));
	invoke_expression = invoke_expr(J_INT, NULL);
	invoke_expression->args_list = &args_list_expression->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke_expression->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	assert_insn_imm(INSN_PUSH, 0x02, insn_entry(bb->insn_list.next));
	assert_insn_imm(INSN_PUSH, 0x01, insn_entry(bb->insn_list.next->next));
	assert_int_equals(INSN_CALL, insn_entry(bb->insn_list.next->next->next)->insn_op);

	free_basic_block(bb);
}
