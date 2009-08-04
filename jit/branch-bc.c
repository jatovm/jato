/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode control transfer
 * instructions to immediate representation of the JIT compiler.
 */

#include "jit/bytecode-to-ir.h"
#include "jit/statement.h"
#include "jit/compiler.h"

#include "vm/bytecodes.h"
#include "vm/bytecode.h"
#include "vm/stack.h"
#include "vm/die.h"

#include <errno.h>

struct statement *if_stmt(struct basic_block *true_bb,
			  enum vm_type vm_type,
			  enum binary_operator binop,
			  struct expression *binary_left,
			  struct expression *binary_right)
{
	struct expression *if_conditional;
	struct statement *if_stmt;

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

static struct statement *__convert_if(struct parse_context *ctx,
				      enum vm_type vm_type,
				      enum binary_operator binop,
				      struct expression *binary_left,
				      struct expression *binary_right)
{
	struct basic_block *true_bb;
	int32_t if_target;

	if_target = bytecode_read_branch_target(ctx->opc, ctx->buffer);
	true_bb = find_bb(ctx->cu, ctx->offset + if_target);

	return if_stmt(true_bb, vm_type, binop, binary_left, binary_right);
}

static int convert_if(struct parse_context *ctx, enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value, *zero_value;

	zero_value = value_expr(J_INT, 0);
	if (!zero_value)
		return warn("out of memory"), -ENOMEM;

	if_value = stack_pop(ctx->bb->mimic_stack);
	stmt = __convert_if(ctx, J_INT, binop, if_value, zero_value);
	if (!stmt) {
		expr_put(zero_value);
		return warn("out of memory"), -ENOMEM;
	}
	convert_statement(ctx, stmt);
	return 0;
}

int convert_ifeq(struct parse_context *ctx)
{
	return convert_if(ctx, OP_EQ);
}

int convert_ifne(struct parse_context *ctx)
{
	return convert_if(ctx, OP_NE);
}

int convert_iflt(struct parse_context *ctx)
{
	return convert_if(ctx, OP_LT);
}

int convert_ifge(struct parse_context *ctx)
{
	return convert_if(ctx, OP_GE);
}

int convert_ifgt(struct parse_context *ctx)
{
	return convert_if(ctx, OP_GT);
}

int convert_ifle(struct parse_context *ctx)
{
	return convert_if(ctx, OP_LE);
}

static int convert_if_cmp(struct parse_context *ctx, enum vm_type vm_type, enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value1, *if_value2;

	if_value2 = stack_pop(ctx->bb->mimic_stack);
	if_value1 = stack_pop(ctx->bb->mimic_stack);

	stmt = __convert_if(ctx, vm_type, binop, if_value1, if_value2);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	convert_statement(ctx, stmt);
	return 0;
}

int convert_if_icmpeq(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_INT, OP_EQ);
}

int convert_if_icmpne(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_INT, OP_NE);
}

int convert_if_icmplt(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_INT, OP_LT);
}

int convert_if_icmpge(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_INT, OP_GE);
}

int convert_if_icmpgt(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_INT, OP_GT);
}

int convert_if_icmple(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_INT, OP_LE);
}

int convert_if_acmpeq(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_REFERENCE, OP_EQ);
}

int convert_if_acmpne(struct parse_context *ctx)
{
	return convert_if_cmp(ctx, J_REFERENCE, OP_NE);
}

int convert_goto(struct parse_context *ctx)
{
	struct basic_block *target_bb;
	struct statement *goto_stmt;
	int32_t goto_target;

	goto_target = bytecode_read_branch_target(ctx->opc, ctx->buffer);

	target_bb = find_bb(ctx->cu, goto_target + ctx->offset);

	goto_stmt = alloc_statement(STMT_GOTO);
	if (!goto_stmt)
		return warn("out of memory"), -ENOMEM;

	goto_stmt->goto_target = target_bb;
	convert_statement(ctx, goto_stmt);
	return 0;
}

int convert_ifnull(struct parse_context *ctx)
{
	return convert_if(ctx, OP_EQ);
}

int convert_ifnonnull(struct parse_context *ctx)
{
	return convert_if(ctx, OP_NE);
}
