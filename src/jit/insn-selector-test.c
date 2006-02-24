/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <basic-block.h>
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

static struct insn *to_insn(struct list_head *head)
{
	return list_entry(head, struct insn, insn_list_node);
}

static void assert_rewrite_binop_expr(unsigned long target,
				      unsigned long left_local,
				      unsigned long right_local)
{
	struct basic_block *bb = alloc_basic_block(0, 1);
	struct expression *expr;

	expr = binop_expr(J_INT, ADD, local_expr(J_INT, left_local),
			  local_expr(J_INT, right_local));
	insn_select(bb, expr);
	assert_insn(MOV, left_local, target, to_insn(bb->insn_list.next));
	assert_insn(ADD, right_local, target, to_insn(bb->insn_list.next->next));
	expr_put(expr);
	free_basic_block(bb);
}

void test_rewrite_add_expr(void)
{
	assert_rewrite_binop_expr(0, 1, 2);
}
