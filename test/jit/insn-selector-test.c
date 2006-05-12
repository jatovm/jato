/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <basic-block.h>
#include <jit/expression.h>
#include <instruction.h>
#include <insn-selector.h>
#include <jit/statement.h>
#include <jit-compiler.h>

static void assert_disp_reg_insn(enum insn_opcode insn_op,
				    enum reg src_base_reg,
				    unsigned long src_displacement,
				    enum reg dest_reg, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE_2(insn_op, AM_DISP, AM_REG), insn->type);
	assert_int_equals(src_base_reg, insn->src.base_reg);
	assert_int_equals(src_displacement, insn->src.disp);
	assert_int_equals(dest_reg, insn->dest.reg);
}

static void assert_reg_insn(enum insn_opcode expected_opc,
			    enum reg expected_reg, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE(expected_opc, AM_REG), insn->type);
	assert_int_equals(expected_reg, insn->operand.reg);
}

static void assert_imm_insn(enum insn_opcode expected_opc,
			    unsigned long expected_imm, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE(expected_opc, AM_IMM), insn->type);
	assert_int_equals(expected_imm, insn->operand.imm);
}

static void assert_rel_insn(enum insn_opcode expected_opc,
			    unsigned long expected_imm, struct insn *insn)
{
	assert_int_equals(DEFINE_INSN_TYPE(expected_opc, AM_REL), insn->type);
	assert_int_equals(expected_imm, insn->operand.rel);
}

static struct insn *insn_next(struct insn *insn)
{
	return insn_entry(insn->insn_list_node.next);
}

static void assert_add_insns(enum reg dest_reg,
			     unsigned long left_local,
			     unsigned long right_local,
			     unsigned long left_displacement,
			     unsigned long right_displacement)
{
	struct insn *insn;
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *expr;
	struct statement *stmt;

	expr = binop_expr(J_INT, OP_ADD, local_expr(J_INT, left_local),
			  local_expr(J_INT, right_local));
	stmt = alloc_statement(STMT_RETURN);
	stmt->return_value = &expr->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	insn = insn_entry(bb->insn_list.next);
	assert_disp_reg_insn(OPC_MOV, REG_EBP, right_displacement, REG_EAX,
				insn);

	insn = insn_next(insn);
	assert_disp_reg_insn(OPC_ADD, REG_EBP, left_displacement, REG_EAX,
				insn);

	free_basic_block(bb);
}

void test_select_insns_for_add(void)
{
	assert_add_insns(REG_EAX, 0, 1, 8, 12);
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

void test_select_insn_for_invoke_without_args(void)
{
	struct methodblock mb;
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *expr, *args_list;
	struct statement *stmt;
	struct insn *insn;

	args_list = no_args_expr();
	expr = invoke_expr(J_INT, &mb);
	expr->args_list = &args_list->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &expr->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	insn = insn_entry(bb->insn_list.next);
	assert_rel_insn(OPC_CALL, (unsigned long) mb.trampoline->objcode, insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
	free_basic_block(bb);
}

void test_select_insn_for_invoke_with_args_list(void)
{
	struct methodblock mb;
	struct insn *insn;
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *invoke_expression, *args_list_expression;
	struct statement *stmt;

	args_list_expression = args_list_expr(arg_expr(value_expr(J_INT, 0x02)),
					      arg_expr(value_expr
						       (J_INT, 0x01)));
	invoke_expression = invoke_expr(J_INT, &mb);
	invoke_expression->args_list = &args_list_expression->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke_expression->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	insn = insn_entry(bb->insn_list.next);
	assert_imm_insn(OPC_PUSH, 0x02, insn);

	insn = insn_next(insn);
	assert_imm_insn(OPC_PUSH, 0x01, insn);

	insn = insn_next(insn);
	assert_rel_insn(OPC_CALL, (unsigned long) mb.trampoline->objcode, insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
	free_basic_block(bb);
}

void test_should_push_invoke_return_value_as_parameter(void)
{
	struct expression *no_args, *arg, *invoke, *nested_invoke;
	struct statement *stmt;
	struct methodblock mb, nested_mb;
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct insn *insn;

	no_args = no_args_expr();
	nested_invoke = invoke_expr(J_INT, &nested_mb);
	nested_invoke->args_list = &no_args->node;

	arg = arg_expr(nested_invoke);
	invoke = invoke_expr(J_INT, &mb);
	invoke->args_list = &arg->node;

	stmt = alloc_statement(STMT_EXPRESSION);
	stmt->expression = &invoke->node;
	bb_insert_stmt(bb, stmt);

	insn_select(bb);

	insn = insn_entry(bb->insn_list.next);
	assert_rel_insn(OPC_CALL, (unsigned long) nested_mb.trampoline->objcode, insn);

	insn = insn_next(insn);
	assert_reg_insn(OPC_PUSH, REG_EAX, insn);

	insn = insn_next(insn);
	assert_rel_insn(OPC_CALL, (unsigned long) mb.trampoline->objcode, insn);

	free_jit_trampoline(mb.trampoline);
	free_compilation_unit(mb.compilation_unit);
	free_basic_block(bb);
}
