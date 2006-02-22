/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <expression.h>
#include <instruction.h>
#include <insn-selector.h>

static void assert_insn(enum insn_opcode insn_op,
			unsigned long src, unsigned long dest,
			struct insn *insn)
{
	assert_int_equals(insn_op, insn->insn_op);
	assert_int_equals(src, insn->src);
	assert_int_equals(dest, insn->dest);
}

static void assert_rewrite_binop_expr(unsigned long left, unsigned long right)
{
	struct insn *insn;
	struct expression *expr;

	expr = binop_expr(J_INT, ADD, temporary_expr(J_INT, left),
			  temporary_expr(J_INT, right));
	insn = insn_select(expr);
	assert_insn(ADD, right, left, insn);
	free_insn(insn);
	expr_put(expr);
}

void test_rewrite_add_expr(void)
{
	assert_rewrite_binop_expr(1, 2);
	assert_rewrite_binop_expr(2, 1);
}
