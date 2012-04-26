/*
 * Convert bytecode to register-based immediate representation.
 *
 * Copyright (c) 2005, 2006, 2009  Pekka Enberg
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

#include "jit/bc-offset-mapping.h"

#include "jit/bytecode-to-ir.h"
#include "jit/expression.h"
#include "jit/subroutine.h"
#include "jit/statement.h"
#include "jit/tree-node.h"
#include "jit/compiler.h"

#include "vm/bytecode.h"
#include "vm/method.h"
#include "vm/die.h"

#include "lib/stack.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define convert_goto_w		convert_goto

#define convert_areturn convert_xreturn
#define convert_dreturn convert_xreturn
#define convert_freturn convert_xreturn
#define convert_ireturn convert_xreturn
#define convert_lreturn convert_xreturn

#define convert_dcmpg convert_xcmpg
#define convert_fcmpg convert_xcmpg
#define convert_dcmpl convert_xcmpl
#define convert_fcmpl convert_xcmpl

#define BYTECODE(opc, name, size, type) [opc] = convert_ ## name,
static convert_fn_t converters[] = {
#  include <vm/bytecode-def.h>
};
#undef BYTECODE

/*
 * Here begins the actual BC2IR algorithm.
 */

void convert_expression(struct parse_context *ctx, struct expression *expr)
{
	tree_patch_bc_offset(&expr->node, ctx->offset);

	/* byte, char and short are always pushed as ints */
	expr->vm_type = mimic_stack_type(expr->vm_type);

	stack_push(ctx->bb->mimic_stack, expr);
}

void do_convert_statement(struct basic_block *bb, struct statement *stmt,
			  unsigned long bc_offset)
{
	tree_patch_bc_offset(&stmt->node, bc_offset);
	bb_add_stmt(bb, stmt);
}

void convert_statement(struct parse_context *ctx, struct statement *stmt)
{
	do_convert_statement(ctx->bb, stmt, ctx->offset);
}

static int spill_expression(struct basic_block *bb,
			    struct stack *reload_stack,
			    struct expression *expr, int slot_ndx)
{
	struct statement *store, *branch;
	struct expression *exit_tmp, *entry_tmp;

	exit_tmp = mimic_stack_expr(expr->vm_type, 0, slot_ndx);
	entry_tmp = mimic_stack_expr(expr->vm_type, 1, slot_ndx);

	if (!exit_tmp || !entry_tmp)
		return warn("out of memory"), -ENOMEM;

	bb_add_mimic_stack_expr(bb, exit_tmp);

	store = alloc_statement(STMT_STORE);
	if (!store)
		return warn("out of memory"), -ENOMEM;

	store->store_dest = &exit_tmp->node;
	store->store_src = &expr->node;

	if (bb->has_branch)
		branch = bb_remove_last_stmt(bb);
	else
		branch = NULL;

	/*
	 * Insert spill store to the current basic block.
	 */
	do_convert_statement(bb, store, bb->end);

	if (branch)
		bb_add_stmt(bb, branch);

	/*
	 * And add a reload expression that is put on the mimic stack of
	 * successor basic blocks.
	 */
	stack_push(reload_stack, expr_get(entry_tmp));

	return 0;
}

static struct stack *spill_mimic_stack(struct basic_block *bb)
{
	struct stack *reload_stack;
	int slot_ndx = 0;

	reload_stack = alloc_stack();
	if (!reload_stack)
		return NULL;

	/*
	 * The reload stack contains elements in reverse order of the mimic
	 * stack.
	 */
	while (!stack_is_empty(bb->mimic_stack)) {
		struct expression *expr;
		int err;

		expr = stack_pop(bb->mimic_stack);

		err = spill_expression(bb, reload_stack, expr, slot_ndx);
		if (err)
			goto error_oom;

		slot_ndx++;
	}
	return reload_stack;
error_oom:
	free_stack(reload_stack);
	return NULL;
}

int convert_instruction(struct parse_context *ctx)
{
	convert_fn_t convert;
	int err;

	ctx->opc = bytecode_read_u8(ctx->buffer);

	if (ctx->opc >= ARRAY_SIZE(converters))
		return warn("%d out of bounds", ctx->opc), -EINVAL;

	convert = converters[ctx->opc];
	if (!convert)
		return warn("no converter for %d found", ctx->opc), -EINVAL;

	if ((ctx->opc >= OPC_IALOAD && ctx->opc <= OPC_SALOAD)
		|| (ctx->opc >= OPC_IASTORE && ctx->opc <= OPC_SASTORE))
		ctx->cu->flags |= CU_FLAG_ARRAY_OPC;
	err = convert(ctx);

	if (err) {
		struct vm_method *vmm = ctx->cu->method;

		warn("%s.%s%s: conversion error at PC=%lu", vmm->class->name, vmm->name, vmm->type, ctx->offset);
	}

	return err;
}

static int do_convert_bb_to_ir(struct basic_block *bb)
{
	struct compilation_unit *cu = bb->b_parent;
	struct bytecode_buffer buffer = { };
	struct parse_context ctx = {
		.buffer = &buffer,
		.cu = cu,
		.bb = bb,
		.code = cu->method->code_attribute.code,
		.is_wide = false,
	};
	int err = 0;

	buffer.buffer = cu->method->code_attribute.code;
	buffer.pos = bb->start;

	if (bb->is_eh && !bb_mimic_stack_is_resolved(bb)) {
		stack_push(bb->mimic_stack, exception_ref_expr());
		bb->entry_mimic_stack_size = 1;
	}

	if (!bb_mimic_stack_is_resolved(bb))
		bb->entry_mimic_stack_size = 0;

	while (buffer.pos < bb->end) {
		ctx.offset = ctx.buffer->pos;	/* this is fragile */

		err = convert_instruction(&ctx);
		if (err)
			break;
	}
	return err;
}

static int reload_mimic_stack(struct basic_block *bb, struct stack *reload)
{
	unsigned int i;

	for (i = 0; i < reload->nr_elements; i++)
		bb_add_mimic_stack_expr(bb, reload->elements[i]);

	if (bb_mimic_stack_is_resolved(bb)) {
		if (stack_size(reload) == (unsigned long) bb->entry_mimic_stack_size)
			return 0;

		return warn("stack size differs on different paths"), -EINVAL;
	}

	for (i = 0; i < reload->nr_elements; i++) {
		struct expression *elem;

		elem = reload->elements[reload->nr_elements - i - 1];
		expr_get(elem);
		stack_push(bb->mimic_stack, elem);
	}

	bb->entry_mimic_stack_size = stack_size(bb->mimic_stack);
	return 0;
}

static int convert_bb_to_ir(struct basic_block *bb)
{
	struct stack *reload_stack;
	unsigned int i;
	int err;

	if (bb->is_converted)
		return 0;

	err = do_convert_bb_to_ir(bb);
	if (err)
		return err;

	bb->is_converted = true;

	/*
	 * If there are no successors, avoid spilling mimic stack contents.
	 */
	if (bb->nr_successors == 0)
		goto out;

	/*
	 * The operand stack can be non-empty at the entry or exit of a basic
	 * block because of, for example, ternary operator. To guarantee that
	 * the mimic stack operands are the same at the merge points of all
	 * paths, we spill any remaining values at the end of a basic block in
	 * memory locations and initialize the mimic stack of any successor
	 * basic blocks to load those values from memory.
	 */
	reload_stack = spill_mimic_stack(bb);
	if (!reload_stack)
		return warn("out of memory"), -ENOMEM;

	for (i = 0; i < bb->nr_successors; i++)
		reload_mimic_stack(bb->successors[i], reload_stack);

	clear_mimic_stack(reload_stack);
	free_stack(reload_stack);

	for (i = 0; i < bb->nr_successors; i++) {
		struct basic_block *s = bb->successors[i];

		err = convert_bb_to_ir(s);
		if (err)
			break;
	}
out:
	return err;
}

static void
assign_temporary(struct basic_block *bb, int entry, int slot_ndx,
		 struct var_info *tmp_high, struct var_info *tmp_low)
{
	struct expression *expr;
	unsigned int i;

	for (i = 0; i < bb->nr_mimic_stack_expr; i++) {
		expr = bb->mimic_stack_expr[i];

		if (expr_type(expr) != EXPR_MIMIC_STACK_SLOT)
			continue;

		if (expr->entry != entry ||
				expr->slot_ndx != slot_ndx)
			continue;

		if (vm_type_is_float(expr->vm_type))
			expr_set_type(expr, EXPR_FLOAT_TEMPORARY);
		else
			expr_set_type(expr, EXPR_TEMPORARY);

#ifdef CONFIG_32_BIT
		expr->tmp_high = tmp_high;
#endif
		expr->tmp_low = tmp_low;
	}
}

static void propagate_temporary(struct basic_block *bb, bool entry, int slot_ndx,
		struct var_info *tmp_high, struct var_info *tmp_low, struct basic_block *from)
{
	struct basic_block **neighbors;
	int nr_neighbors;
	int j;

	if (entry) {
		neighbors    = bb->predecessors;
		nr_neighbors = bb->nr_predecessors;
	} else {
		neighbors    = bb->successors;
		nr_neighbors = bb->nr_successors;
	}

	assign_temporary(bb, entry, slot_ndx, tmp_high, tmp_low);

	for (j = 0; j < nr_neighbors; j++) {
		if (neighbors[j] == from)
			continue;

		propagate_temporary(neighbors[j], !entry, slot_ndx, tmp_high, tmp_low, bb);
	}
}

static void pick_and_propagate_temporaries(struct basic_block *bb, bool entry)
{
	struct var_info *tmp_high, *tmp_low;
	struct expression *expr;
	unsigned int i;

	for (i = 0; i < bb->nr_mimic_stack_expr; i++) {
		expr = bb->mimic_stack_expr[i];

		/* Skip expressions that already been transformed */
		if (expr_type(expr) != EXPR_MIMIC_STACK_SLOT)
			continue;

		/* Skip slots related to the exit when treating entrance
		 * and vice versa */
		if (expr->entry != entry)
			continue;

		if (expr->vm_type == J_LONG) {
			tmp_high = get_var(bb->b_parent, J_INT);
			tmp_low = get_var(bb->b_parent, J_INT);
		} else {
			tmp_low = get_var(bb->b_parent, expr->vm_type);
			tmp_high = NULL;
		}

		/* Assign this temporary to same mimic stack expressions in this block and its neighbors */
		propagate_temporary(bb, entry, expr->slot_ndx, tmp_high, tmp_low, bb);
	}
}

static bool
need_to_resolve(int nr_neighbors, struct basic_block **neighbors, int entry)
{
	/* No successors? We have nothing to do */
	if (!nr_neighbors)
		return false;

	/* Are we a slave? */
	if (nr_neighbors == 1) {
		struct basic_block *n = neighbors[0];
		int nr_connect;

		if (entry)
			nr_connect = n->nr_successors;
		else
			nr_connect = n->nr_predecessors;

		if (nr_connect == 1) {
			/* Slave-slave relationship, we can pick a temporary */
			return true;
		} else {
			/* Slave-master, do nothing */
			return false;
		}
	} else {
		/* A master always picks his temporaries */
		return true;
	}
}

/**
 * resolve_mimic_stack_slots - Transform the mimic stack slots expressions
 * of the basic bloc into temporary expressions, based on
 * master/slave properties depending on the number of successors and predecessors.
 */
static int resolve_mimic_stack_slots(struct basic_block *bb)
{
	if (need_to_resolve(bb->nr_successors, bb->successors, 0))
		pick_and_propagate_temporaries(bb, false);

	if (need_to_resolve(bb->nr_predecessors, bb->predecessors, 1))
		pick_and_propagate_temporaries(bb, true);

	return 0;
}

/**
 *	convert_to_ir - Convert bytecode to intermediate representation.
 *	@compilation_unit: compilation unit to convert.
 *
 *	This function converts bytecode in a compilation unit to intermediate
 *	representation of the JIT compiler.
 *
 *	Returns zero if conversion succeeded; otherwise returns a negative
 * 	integer.
 */
int convert_to_ir(struct compilation_unit *cu)
{
	struct basic_block *bb;
	int err;

	err = convert_bb_to_ir(cu->entry_bb);
	if (err)
		return err;

	/*
	 * A compilation unit can have exception handler basic blocks that are
	 * not reachable from the entry basic block. Therefore, make sure we've
	 * really converted all basic blocks.
	 */
	for_each_basic_block(bb, &cu->bb_list) {
		if (!bb->is_eh)
			continue;

		err = convert_bb_to_ir(bb);
		if (err)
			break;
	}

	/*
	 * Connect mimic stacks between basic blocks by changing
	 * each EXPR_MIMIC_STACK_SLOT into an EXPR_TEMPORARY
	 */
	for_each_basic_block(bb, &cu->bb_list) {
		err = resolve_mimic_stack_slots(bb);
		if (err)
			break;
	}

	return err;
}
