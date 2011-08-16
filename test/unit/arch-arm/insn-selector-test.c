#include "jit/compilation-unit.h"
#include "jit/statement.h"
#include "jit/instruction.h"
#include "jit/expression.h"
#include "jit/basic-block.h"
#include "jit/vars.h"
#include "arch/instruction.h"
#include "lib/list.h"
#include "jit/emit-code.h"
#include "lib/buffer.h"

#include <libharness.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>

void test_expr_value_pool(void)
{
	struct compilation_unit *cu = stub_compilation_unit_alloc();
	struct basic_block *bb = alloc_basic_block(cu, 1, 40);
	struct statement *stmt1 = alloc_statement(STMT_EXPRESSION);
	struct statement *stmt2 = alloc_statement(STMT_EXPRESSION);
	struct statement *stmt3 = alloc_statement(STMT_EXPRESSION);

	struct expression *expr1 = value_expr(J_INT, 2000);
	struct expression *expr2 = value_expr(J_LONG, 0x7861334578690);
	struct expression *expr3 = value_expr(J_INT, 2000);
	struct insn *insn;

	int value[4];
	int index[4];
	int i = 0;
	stmt1->expression = &expr1->node;
	stmt2->expression = &expr2->node;
	stmt3->expression = &expr3->node;

	bb_add_stmt(bb, stmt1);
	bb_add_stmt(bb, stmt2);
	bb_add_stmt(bb, stmt3);

	insn_select(bb);

	for_each_insn(insn, &bb->insn_list) {
		value[i] = insn->src.pool->value;
		index[i] = insn->src.pool->index;
		i++;
	}

	assert_int_equals(2000, value[0]);
	assert_int_equals(878151312, value[1]);
	assert_int_equals(493075, value[2]);
	assert_int_equals(2000, value[3]);
	assert_int_equals(0, index[0]);
	assert_int_equals(1, index[1]);
	assert_int_equals(2, index[2]);
	assert_int_equals(0, index[3]);

	free_basic_block(bb);
}

void test_expr_value_imm(void)
{
	struct compilation_unit *cu = stub_compilation_unit_alloc();
	struct basic_block *bb = alloc_basic_block(cu, 1, 20);
	struct statement *stmt = alloc_statement(STMT_EXPRESSION);

	struct expression *expr = value_expr(J_INT, 0);
	struct insn *insn;
	stmt->expression = &expr->node;

	bb_add_stmt(bb, stmt);

	insn_select(bb);

	for_each_insn(insn, &bb->insn_list) {
		assert_int_equals(0, insn->src.imm);
	}

	free_basic_block(bb);
}

void test_expr_local(void)
{
	struct compilation_unit *cu = stub_compilation_unit_alloc();
	struct stack_frame *frame = alloc_stack_frame(0, 2);
	cu->stack_frame = frame;

	struct basic_block *bb = alloc_basic_block(cu, 1, 20);
	struct statement *stmt1 = alloc_statement(STMT_EXPRESSION);
	struct statement *stmt2 = alloc_statement(STMT_EXPRESSION);

	struct expression *expr1 = local_expr(J_INT, 0);
	struct expression *expr2 = local_expr(J_LONG, 0);
	struct insn *insn;

	unsigned long index[3];
	short i = 0;

	stmt1->expression = &expr1->node;
	stmt2->expression = &expr2->node;

	bb_add_stmt(bb, stmt1);
	bb_add_stmt(bb, stmt2);

	insn_select(bb);
	for_each_insn(insn, &bb->insn_list) {
		index[i] = insn->src.slot->index;
		i++;
	}

	assert_int_equals(0, index[0]);
	assert_int_equals(0, index[1]);
	assert_int_equals(1, index[2]);

	free_basic_block(bb);
}

void test_stmt_store(void)
{
	struct compilation_unit *cu = stub_compilation_unit_alloc();
	struct stack_frame *frame = alloc_stack_frame(3, 5);
	cu->stack_frame = frame;

	struct basic_block *bb = alloc_basic_block(cu, 1, 20);
	struct statement *stmt1 = alloc_statement(STMT_STORE);
	struct statement *stmt2 = alloc_statement(STMT_STORE);

	struct expression *expr1 = local_expr(J_INT, 3);
	struct expression *expr2 = local_expr(J_LONG, 3);
	struct expression *expr3 = value_expr(J_INT, 100);
	struct expression *expr4 = value_expr(J_LONG, 0x0000003400000016);
	struct insn *insn;

	stmt1->store_dest = &expr1->node;
	stmt1->store_src = &expr3->node;
	stmt2->store_dest = &expr2->node;
	stmt2->store_src = &expr4->node;

	bb_add_stmt(bb, stmt1);
	bb_add_stmt(bb, stmt2);

	int index[6];
	int i = 0;

	insn_select(bb);
	for_each_insn(insn, &bb->insn_list) {
		if (insn->type == INSN_STR_MEMLOCAL_REG)
			index[i] = insn->dest.slot->index;
		else
			index[i] = insn->src.imm;

		i++;
	}

	assert_int_equals(100, index[0]);
	assert_int_equals(3, index[1]);
	assert_int_equals(22, index[2]);
	assert_int_equals(52, index[3]);
	assert_int_equals(3, index[4]);
	assert_int_equals(4, index[5]);

	free_basic_block(bb);
}
