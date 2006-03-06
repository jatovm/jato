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

static void assert_rewrite_binop_expr(enum reg dest_reg,
				      unsigned long left_local,
				      unsigned long right_local,
				      unsigned long left_displacement,
				      unsigned long right_displacement)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *expr;
	struct statement *stmt;

	expr = binop_expr(J_INT, ADD, local_expr(J_INT, left_local),
			  local_expr(J_INT, right_local));
	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = expr;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);
	assert_insn(MOV, REG_EBP, right_displacement, REG_EAX, insn_entry(bb->insn_list.next));
	assert_insn(ADD, REG_EBP, left_displacement, REG_EAX, insn_entry(bb->insn_list.next->next));

	free_basic_block(bb);
}

void test_rewrite_add_expr(void)
{
	assert_rewrite_binop_expr(REG_EAX, 0, 1, 8, 12);
}

void test_select_insn_for_invoke_without_args(void)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct statement *stmt;
	struct insn *insn;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = invoke_expr(J_INT, NULL);
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	insn = insn_entry(bb->insn_list.next);
	assert_int_equals(INSN_CALL, insn->insn_op);

	free_basic_block(bb);
}

void test_select_insn_for_args_list(void)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct statement *stmt;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = args_list_expr(
		arg_expr(value_expr(J_INT, 0x01)),
		arg_expr(value_expr(J_INT, 0x02)));
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	assert_int_equals(INSN_PUSH, insn_entry(bb->insn_list.next)->insn_op);
	assert_int_equals(INSN_PUSH, insn_entry(bb->insn_list.next->next)->insn_op);

	free_basic_block(bb);
}
