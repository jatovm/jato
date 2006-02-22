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

void test_insn_select(void)
{
	struct insn *insn;
	struct expression *expr;

	expr = binop_expr(J_INT, ADD, temporary_expr(J_INT, 1),
			  temporary_expr(J_INT, 2));
	insn = insn_select(expr);
	assert_insn(ADD, 2, 1, insn);
	free_insn(insn);
	expr_put(expr);
}
