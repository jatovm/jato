/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include "jit/bytecode-to-ir.h"
#include "jit/statement.h"
#include "jit/compiler.h"

#include "vm/bytecodes.h"
#include "vm/bytecode.h"
#include "vm/stack.h"

#include <errno.h>

static int convert_binop(struct parse_context *ctx, enum vm_type vm_type,
			 enum binary_operator binary_operator)
{
	struct expression *left, *right, *expr;

	right = stack_pop(ctx->bb->mimic_stack);
	left = stack_pop(ctx->bb->mimic_stack);

	expr = binop_expr(vm_type, binary_operator, left, right);
	if (!expr)
		return -ENOMEM;

	convert_expression(ctx, expr);
	return 0;
}

int convert_iadd(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_ADD);
}

int convert_ladd(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_ADD);
}

int convert_fadd(struct parse_context *ctx)
{
	return convert_binop(ctx, J_FLOAT, OP_FADD);
}

int convert_dadd(struct parse_context *ctx)
{
	return convert_binop(ctx, J_DOUBLE, OP_ADD);
}

int convert_isub(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_SUB);
}

int convert_lsub(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_SUB);
}

int convert_fsub(struct parse_context *ctx)
{
	return convert_binop(ctx, J_FLOAT, OP_FSUB);
}

int convert_dsub(struct parse_context *ctx)
{
	return convert_binop(ctx, J_DOUBLE, OP_SUB);
}

int convert_imul(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_MUL);
}

int convert_lmul(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_MUL_64);
}

int convert_fmul(struct parse_context *ctx)
{
	return convert_binop(ctx, J_FLOAT, OP_FMUL);
}

int convert_dmul(struct parse_context *ctx)
{
	return convert_binop(ctx, J_DOUBLE, OP_MUL);
}

int convert_idiv(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_DIV);
}

int convert_ldiv(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_DIV_64);
}

int convert_fdiv(struct parse_context *ctx)
{
	return convert_binop(ctx, J_FLOAT, OP_FDIV);
}

int convert_ddiv(struct parse_context *ctx)
{
	return convert_binop(ctx, J_DOUBLE, OP_DIV);
}

int convert_irem(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_REM);
}

int convert_lrem(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_REM_64);
}

int convert_frem(struct parse_context *ctx)
{
	return convert_binop(ctx, J_FLOAT, OP_FREM);
}

int convert_drem(struct parse_context *ctx)
{
	return convert_binop(ctx, J_DOUBLE, OP_REM);
}

static int convert_unary_op(struct parse_context *ctx, enum vm_type vm_type,
			    enum unary_operator unary_operator)
{
	struct expression *expression, *expr;

	expression = stack_pop(ctx->bb->mimic_stack);

	expr = unary_op_expr(vm_type, unary_operator, expression);
	if (!expr)
		return -ENOMEM;

	convert_expression(ctx, expr);
	return 0;
}

int convert_ineg(struct parse_context *ctx)
{
	return convert_unary_op(ctx, J_INT, OP_NEG);
}

int convert_lneg(struct parse_context *ctx)
{
	return convert_unary_op(ctx, J_LONG, OP_NEG);
}

int convert_fneg(struct parse_context *ctx)
{
	return convert_unary_op(ctx, J_FLOAT, OP_FNEG);
}

int convert_dneg(struct parse_context *ctx)
{
	return convert_unary_op(ctx, J_DOUBLE, OP_FNEG);
}

int convert_ishl(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_SHL);
}

int convert_lshl(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_SHL_64);
}

int convert_ishr(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_SHR);
}

int convert_lshr(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_SHR_64);
}

int convert_iushr(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_USHR);
}

int convert_lushr(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_USHR_64);
}

int convert_iand(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_AND);
}

int convert_land(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_AND);
}

int convert_ior(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_OR);
}

int convert_lor(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_OR);
}

int convert_ixor(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_XOR);
}

int convert_lxor(struct parse_context *ctx)
{
	return convert_binop(ctx, J_LONG, OP_XOR);
}

int convert_iinc(struct parse_context *ctx)
{
	struct statement *store_stmt;
	struct expression *local_expression, *binop_expression,
	    *const_expression;
	unsigned int index;
	int const_value;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	if (ctx->is_wide) {
		index = bytecode_read_u16(ctx->buffer);
		const_value = bytecode_read_s16(ctx->buffer);
	} else {
		index = bytecode_read_u8(ctx->buffer);
		const_value = bytecode_read_s8(ctx->buffer);
	}

	local_expression = local_expr(J_INT, index);
	if (!local_expression)
		goto failed;

	store_stmt->store_dest = &local_expression->node;

	const_expression = value_expr(J_INT, const_value);
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
	convert_statement(ctx, store_stmt);

	return 0;

      failed:
	free_statement(store_stmt);
	return -ENOMEM;
}

int convert_lcmp(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_CMP);
}

int convert_xcmpl(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_CMPL);
}

int convert_xcmpg(struct parse_context *ctx)
{
	return convert_binop(ctx, J_INT, OP_CMPG);
}
