/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode control transfer
 * instructions to immediate representation of the JIT compiler.
 */

#include <jit/jit-compiler.h>
#include <jit/statement.h>

#include <vm/bytecodes.h>
#include <vm/stack.h>

#include <errno.h>

static struct statement *__convert_if(struct compilation_unit *cu,
				      unsigned long offset,
				      enum vm_type vm_type,
				      enum binary_operator binop,
				      struct expression *binary_left,
				      struct expression *binary_right)
{
	struct basic_block *true_bb;
	struct expression *if_conditional;
	struct statement *if_stmt;
	unsigned long if_target;

	if_target = bytecode_br_target(cu->method->jit_code + offset);
	true_bb = find_bb(cu, offset + if_target);

	if_conditional = binop_expr(vm_type, binop, binary_left, binary_right);
	if (!if_conditional)
		goto failed;

	if_stmt = alloc_statement(STMT_IF);
	if (!if_stmt)
		goto failed_put_expr;

	if_stmt->if_true = true_bb;
	if_stmt->if_conditional = &if_conditional->node;

	return if_stmt;
      failed_put_expr:
	expr_put(if_conditional);
      failed:
	return NULL;
}

static int convert_if(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset,
		      enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value, *zero_value;

	zero_value = value_expr(J_INT, 0);
	if (!zero_value)
		return -ENOMEM;

	if_value = stack_pop(cu->expr_stack);
	stmt = __convert_if(cu, offset, J_INT, binop, if_value, zero_value);
	if (!stmt) {
		expr_put(zero_value);
		return -ENOMEM;
	}
	bb_add_stmt(bb, stmt);
	return 0;
}

int convert_ifeq(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_EQ);
}

int convert_ifne(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_NE);
}

int convert_iflt(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_LT);
}

int convert_ifge(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_GE);
}

int convert_ifgt(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_GT);
}

int convert_ifle(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_LE);
}

static int convert_if_cmp(struct compilation_unit *cu,
			  struct basic_block *bb,
			  unsigned long offset,
			  enum vm_type vm_type, enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value1, *if_value2;

	if_value2 = stack_pop(cu->expr_stack);
	if_value1 = stack_pop(cu->expr_stack);

	stmt = __convert_if(cu, offset, vm_type, binop, if_value1, if_value2);
	if (!stmt)
		return -ENOMEM;

	bb_add_stmt(bb, stmt);
	return 0;
}

int convert_if_icmpeq(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_EQ);
}

int convert_if_icmpne(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_NE);
}

int convert_if_icmplt(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_LT);
}

int convert_if_icmpge(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_GE);
}

int convert_if_icmpgt(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_GT);
}

int convert_if_icmple(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_LE);
}

int convert_if_acmpeq(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_REFERENCE, OP_EQ);
}

int convert_if_acmpne(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_REFERENCE, OP_NE);
}

int convert_goto(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	struct basic_block *target_bb;
	struct statement *goto_stmt;
	unsigned long goto_target;

	goto_target = bytecode_br_target(cu->method->jit_code + offset);
	target_bb = find_bb(cu, goto_target + offset);

	goto_stmt = alloc_statement(STMT_GOTO);
	if (!goto_stmt)
		return -ENOMEM;

	goto_stmt->goto_target = target_bb;
	bb_add_stmt(bb, goto_stmt);
	return 0;
}

int convert_ifnull(struct compilation_unit *cu, struct basic_block *bb,
		   unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_EQ);
}

int convert_ifnonnull(struct compilation_unit *cu, struct basic_block *bb,
		   unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_NE);
}
