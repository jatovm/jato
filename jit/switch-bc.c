/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <errno.h>

#include "jit/bytecode-to-ir.h"
#include "jit/compiler.h"
#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/statement.h"

#include "vm/bytecode.h"
#include "vm/bytecodes.h"
#include "lib/stack.h"
#include "vm/die.h"

static struct statement *branch_if_lesser_stmt(struct basic_block *target,
					       struct expression *left,
					       int32_t right)
{
	struct expression *right_expr;

	right_expr = value_expr(J_INT, right);
	if (!right_expr)
		return NULL;

	return if_stmt(target, J_INT, OP_LT, left, right_expr);
}

static struct statement *branch_if_greater_stmt(struct basic_block *target,
						struct expression *left,
						int32_t right)
{
	struct expression *right_expr;

	right_expr = value_expr(J_INT, right);
	if (!right_expr)
		return NULL;

	return if_stmt(target, J_INT, OP_GT, left, right_expr);
}

static struct statement *branch_if_null_stmt(struct basic_block *target,
					     struct expression *left)
{
	struct expression *right_expr;

	right_expr = value_expr(J_NATIVE_PTR, 0);
	if (!right_expr)
		return NULL;

	return if_stmt(target, J_NATIVE_PTR, OP_EQ, left, right_expr);
}

int convert_tableswitch(struct parse_context *ctx)
{
	struct tableswitch_info info;
	struct tableswitch *table;
	struct basic_block *master_bb;
	struct basic_block *default_bb;
	struct basic_block *b1;
	struct basic_block *b2;

	get_tableswitch_info(ctx->code, ctx->offset, &info);
	ctx->buffer->pos += info.insn_size;

	default_bb = find_bb(ctx->cu, ctx->offset + info.default_target);
	if (!default_bb)
		goto fail_default_bb;

	master_bb = ctx->bb;

	b1 = bb_split(master_bb, ctx->offset);
	b2 = bb_split(b1, ctx->offset);

	assert(b1 && b2);

	b1->is_converted = true;
	b2->is_converted = true;

	bb_add_successor(master_bb, default_bb );
	bb_add_successor(master_bb, b1);

	bb_add_successor(b1, default_bb );
	bb_add_successor(b1, b2);

	for (unsigned int i = 0; i < info.count; i++) {
		struct basic_block *target_bb;
		int32_t target;

		target = read_s32(info.targets + i * 4);
		target_bb = find_bb(ctx->cu, ctx->offset + target);

		bb_add_successor(b2, target_bb);
	}

	table = alloc_tableswitch(&info, ctx->cu, b2, ctx->offset);
	if (!table)
		return -ENOMEM;

	struct statement *if_lesser_stmt;
	struct statement *if_greater_stmt;
	struct statement *stmt;
	struct expression *pure_index;

	pure_index = get_pure_expr(ctx, stack_pop(ctx->bb->mimic_stack));

	if_lesser_stmt =
		branch_if_lesser_stmt(default_bb, pure_index, info.low);
	if (!if_lesser_stmt)
		goto fail_lesser_stmt;

	expr_get(pure_index);
	if_greater_stmt =
		branch_if_greater_stmt(default_bb, pure_index, info.high);
	if (!if_greater_stmt)
		goto fail_greater_stmt;

	stmt = alloc_statement(STMT_TABLESWITCH);
	if (!stmt)
		goto fail_stmt;

	expr_get(pure_index);
	stmt->index = &pure_index->node;
	stmt->table = table;

	do_convert_statement(master_bb, if_lesser_stmt, ctx->offset);
	do_convert_statement(b1, if_greater_stmt, ctx->offset);
	do_convert_statement(b2, stmt, ctx->offset);
	return 0;

 fail_stmt:
	free_statement(if_greater_stmt);
 fail_greater_stmt:
	free_statement(if_lesser_stmt);
 fail_lesser_stmt:
	free_tableswitch(table);
 fail_default_bb:
	return -1;
}

int convert_lookupswitch(struct parse_context *ctx)
{
	struct lookupswitch_info info;
	struct lookupswitch *table;
	struct basic_block *master_bb;
	struct basic_block *default_bb;
	struct basic_block *b1;

	get_lookupswitch_info(ctx->code, ctx->offset, &info);
	ctx->buffer->pos += info.insn_size;

	default_bb = find_bb(ctx->cu, ctx->offset + info.default_target);
	if (!default_bb)
		goto fail_default_bb;

	master_bb = ctx->bb;

	b1 = bb_split(master_bb, ctx->offset);

	assert(b1);

	b1->is_converted = true;

	bb_add_successor(master_bb, default_bb );
	bb_add_successor(master_bb, b1);

	for (unsigned int i = 0; i < info.count; i++) {
		struct basic_block *target_bb;
		int32_t target;

		target = read_lookupswitch_target(&info, i);
		target_bb = find_bb(ctx->cu, ctx->offset + target);

		bb_add_successor(b1, target_bb);
	}

	table = alloc_lookupswitch(&info, ctx->cu, b1, ctx->offset);
	if (!table)
		return -ENOMEM;

	struct statement *if_null_stmt;
	struct statement *stmt;
	struct expression *key;
	struct expression *bsearch;
	struct expression *pure_bsearch;

	key = stack_pop(ctx->bb->mimic_stack);

	bsearch = lookupswitch_bsearch_expr(key, table);
	if (!bsearch)
		return -1;

	pure_bsearch = get_pure_expr(ctx, bsearch);

	if_null_stmt =
		branch_if_null_stmt(default_bb, pure_bsearch);
	if (!if_null_stmt)
		goto fail_null_stmt;

	stmt = alloc_statement(STMT_LOOKUPSWITCH_JUMP);
	if (!stmt)
		goto fail_stmt;

	expr_get(pure_bsearch);
	stmt->lookupswitch_target = &pure_bsearch->node;

	convert_statement(ctx, if_null_stmt);
	do_convert_statement(b1, stmt, ctx->offset);
	return 0;

 fail_stmt:
	free_statement(if_null_stmt);
 fail_null_stmt:
	free_lookupswitch(table);
 fail_default_bb:
	return -1;
}
