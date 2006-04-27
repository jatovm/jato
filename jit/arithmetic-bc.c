/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include <statement.h>
#include <vm/byteorder.h>
#include <vm/stack.h>
#include <jit-compiler.h>
#include <bytecodes.h>

#include <errno.h>

static int convert_binop(struct compilation_unit *cu,
			 struct basic_block *bb,
			 enum jvm_type jvm_type,
			 enum binary_operator binary_operator)
{
	struct expression *left, *right, *expr;

	right = stack_pop(cu->expr_stack);
	left = stack_pop(cu->expr_stack);

	expr = binop_expr(jvm_type, binary_operator, left, right);
	if (!expr)
		return -ENOMEM;

	stack_push(cu->expr_stack, expr);
	return 0;
}

int convert_iadd(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_ADD);
}

int convert_ladd(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_ADD);
}

int convert_fadd(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_ADD);
}

int convert_dadd(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_ADD);
}

int convert_isub(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_SUB);
}

int convert_lsub(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_SUB);
}

int convert_fsub(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_SUB);
}

int convert_dsub(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_SUB);
}

int convert_imul(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_MUL);
}

int convert_lmul(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_MUL);
}

int convert_fmul(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_MUL);
}

int convert_dmul(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_MUL);
}

int convert_idiv(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_DIV);
}

int convert_ldiv(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_DIV);
}

int convert_fdiv(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_DIV);
}

int convert_ddiv(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_DIV);
}

int convert_irem(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_REM);
}

int convert_lrem(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_REM);
}

int convert_frem(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_REM);
}

int convert_drem(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_REM);
}

static int convert_unary_op(struct compilation_unit *cu,
			    struct basic_block *bb,
			    enum jvm_type jvm_type,
			    enum unary_operator unary_operator)
{
	struct expression *expression, *expr;

	expression = stack_pop(cu->expr_stack);

	expr = unary_op_expr(jvm_type, unary_operator, expression);
	if (!expr)
		return -ENOMEM;

	stack_push(cu->expr_stack, expr);
	return 0;
}

int convert_ineg(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_unary_op(cu, bb, J_INT, OP_NEG);
}

int convert_lneg(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_unary_op(cu, bb, J_LONG, OP_NEG);
}

int convert_fneg(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_unary_op(cu, bb, J_FLOAT, OP_NEG);
}

int convert_dneg(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_unary_op(cu, bb, J_DOUBLE, OP_NEG);
}

int convert_ishl(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_SHL);
}

int convert_lshl(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_SHL);
}

int convert_ishr(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_SHR);
}

int convert_lshr(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_SHR);
}

int convert_iand(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_AND);
}

int convert_land(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_AND);
}

int convert_ior(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_OR);
}

int convert_lor(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_OR);
}

int convert_ixor(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_XOR);
}

int convert_lxor(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_XOR);
}

int convert_iinc(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	struct statement *store_stmt;
	struct expression *local_expression, *binop_expression,
	    *const_expression;
	unsigned char *code = cu->method->code;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	local_expression = local_expr(J_INT, code[offset + 1]);
	if (!local_expression)
		goto failed;

	store_stmt->store_dest = &local_expression->node;

	const_expression = value_expr(J_INT, code[offset + 2]);
	if (!const_expression)
		goto failed;

	expr_get(local_expression);

	binop_expression = binop_expr(J_INT, OP_ADD, local_expression,
				      const_expression);
	if (!binop_expression) {
		expr_put(local_expression);
		expr_put(const_expression);
		goto failed;
	}

	store_stmt->store_src = &binop_expression->node;
	bb_insert_stmt(bb, store_stmt);

	return 0;

      failed:
	free_statement(store_stmt);
	return -ENOMEM;
}

int convert_lcmp(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_CMP);
}

int convert_xcmpl(struct compilation_unit *cu, struct basic_block *bb,
		  unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_CMPL);
}

int convert_xcmpg(struct compilation_unit *cu, struct basic_block *bb,
		  unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_CMPG);
}
