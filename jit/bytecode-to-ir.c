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
#include <jit/bc-offset-mapping.h>
#include <jit/expression.h>
#include <jit/statement.h>
#include <jit/tree-node.h>
#include <jit/compiler.h>

#include <vm/bytecode.h>
#include <vm/bytecodes.h>
#include <vm/die.h>
#include <vm/method.h>
#include <vm/stack.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/*
 * The following bytecodes are not supported yet.
 */

static int convert_not_implemented(struct parse_context *ctx)
{
	return warn("bytecode %d is not supported", ctx->opc), -EINVAL;
}

#define convert_jsr		convert_not_implemented
#define convert_ret		convert_not_implemented
#define convert_tableswitch	convert_not_implemented
#define convert_lookupswitch	convert_not_implemented
#define convert_wide		convert_not_implemented
#define convert_goto_w		convert_not_implemented
#define convert_jsr_w		convert_not_implemented

/*
 * This macro magic sets up the converter lookup table.
 */

typedef int (*convert_fn_t) (struct parse_context *);

#define DECLARE_CONVERTER(name) int name(struct parse_context *)

#define BYTECODE(opc, name, size, type) DECLARE_CONVERTER(convert_ ## name);
#  include <vm/bytecode-def.h>
#undef BYTECODE

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
	stack_push(ctx->bb->mimic_stack, expr);
}

static void
do_convert_statement(struct basic_block *bb, struct statement *stmt, unsigned long bc_offset)
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
		return -ENOMEM;

	bb_add_mimic_stack_expr(bb, exit_tmp);

	store = alloc_statement(STMT_STORE);
	if (!store)
		return -ENOMEM;

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

static int do_convert_bb_to_ir(struct basic_block *bb)
{
	struct compilation_unit *cu = bb->b_parent;
	struct bytecode_buffer buffer = { };
	struct parse_context ctx = {
		.buffer = &buffer,
		.cu = cu,
		.bb = bb,
		.code = cu->method->code_attribute.code,
	};
	int err = 0;

	buffer.buffer = cu->method->code_attribute.code;
	buffer.pos = bb->start;

	if (bb->is_eh)
		stack_push(bb->mimic_stack, exception_ref_expr());

	while (buffer.pos < bb->end) {
		convert_fn_t convert;

		ctx.offset = ctx.buffer->pos;	/* this is fragile */
		ctx.opc = bytecode_read_u8(ctx.buffer);

		if (ctx.opc >= ARRAY_SIZE(converters)) {
			warn("%d out of bounds", ctx.opc);
			err = -EINVAL;
			break;
		}

		convert = converters[ctx.opc];
		if (!convert) {
			warn("no converter for %d found", ctx.opc);
			err = -EINVAL;
			break;
		}

		err = convert(&ctx);
		if (err)
			break;
	}
	return err;
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
		return -ENOMEM;

	while (!stack_is_empty(reload_stack)) {
		struct expression *expr = stack_pop(reload_stack);

		for (i = 0; i < bb->nr_successors; i++) {
			struct basic_block *s = bb->successors[i];

			expr_get(expr);

			bb_add_mimic_stack_expr(s, expr);

			stack_push(s->mimic_stack, expr);
		}
		expr_put(expr);
	}
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

		expr_set_type(expr, EXPR_TEMPORARY);
		expr->tmp_high = tmp_high;
		expr->tmp_low = tmp_low;
	}
}

static void pick_and_propagate_temporaries(struct basic_block *bb, bool entry)
{
	struct var_info *tmp_high, *tmp_low;
	struct basic_block **neighbors;
	struct expression *expr;
	int nr_neighbors;
	unsigned int i;
	int slot_ndx;

	if (entry) {
		neighbors    = bb->predecessors;
		nr_neighbors = bb->nr_predecessors;
	} else {
		neighbors    = bb->successors;
		nr_neighbors = bb->nr_successors;
	}

	for (i = 0; i < bb->nr_mimic_stack_expr; i++) {
		int j;

		expr = bb->mimic_stack_expr[i];

		/* Skip expressions that already been transformed */
		if (expr_type(expr) != EXPR_MIMIC_STACK_SLOT)
			continue;

		/* Skip slots related to the exit when treating entrance
		 * and vice versa */
		if (expr->entry != entry)
			continue;

		tmp_low = get_var(bb->b_parent);
		if (expr->vm_type == J_LONG)
			tmp_high = get_var(bb->b_parent);
		else
			tmp_high = NULL;

		/* Save the slot number */
		slot_ndx = expr->slot_ndx;

		/* Assign this temporary to same mimic stack expressions in this block */
		assign_temporary(bb, entry, expr->slot_ndx, tmp_high, tmp_low);

		for (j = 0; j < nr_neighbors; j++)
			assign_temporary(neighbors[j], !entry, slot_ndx, tmp_high, tmp_low);
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
