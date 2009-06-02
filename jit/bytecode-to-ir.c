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

#include <vm/bytecodes.h>
#include <vm/bytecode.h>
#include <vm/stack.h>
#include <vm/die.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/*
 * The following bytecodes are not supported yet.
 */

static int convert_not_implemented(struct parse_context *ctx)
{
	die("bytecode %d is not supported", ctx->opc);

	return -EINVAL;
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
	expr->bytecode_offset = ctx->offset;
	stack_push(ctx->bb->mimic_stack, expr);
}

static void
do_convert_statement(struct basic_block *bb, struct statement *stmt, unsigned short bc_offset)
{
	/*
	 * Some expressions do not go through convert_expression()
	 * so we need to set their bytecode_offset here if it is not set.
	 */
	switch (stmt_type(stmt)) {
	case STMT_STORE:
		tree_patch_bc_offset(stmt->store_dest, bc_offset);
		tree_patch_bc_offset(stmt->store_src, bc_offset);
		break;
	case STMT_IF:
		tree_patch_bc_offset(stmt->if_conditional, bc_offset);
		break;
	case STMT_RETURN:
		tree_patch_bc_offset(stmt->return_value, bc_offset);
		break;
	case STMT_EXPRESSION:
	case STMT_ARRAY_CHECK:
		tree_patch_bc_offset(stmt->expression, bc_offset);
		break;
	case STMT_ATHROW:
		tree_patch_bc_offset(stmt->exception_ref, bc_offset);
		break;
	default: ;
	}

	stmt->bytecode_offset = bc_offset;
	bb_add_stmt(bb, stmt);
}

void convert_statement(struct parse_context *ctx, struct statement *stmt)
{
	do_convert_statement(ctx->bb, stmt, ctx->offset);
}

static int spill_expression(struct basic_block *bb,
			    struct stack *reload_stack,
			    struct expression *expr)
{
	struct compilation_unit *cu = bb->b_parent;
	struct var_info *tmp_low, *tmp_high;
	struct statement *store, *branch;
	struct expression *tmp;

	tmp_low = get_var(cu);
	if (expr->vm_type == J_LONG)
		tmp_high = get_var(cu);
	else
		tmp_high = NULL;

	tmp = temporary_expr(expr->vm_type, tmp_high, tmp_low);
	if (!tmp)
		return -ENOMEM;

	store = alloc_statement(STMT_STORE);
	if (!store)
		return -ENOMEM;

	store->store_dest = &tmp->node;
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
	stack_push(reload_stack, tmp);

	return 0;
}

static struct stack *spill_mimic_stack(struct basic_block *bb)
{
	struct stack *reload_stack;

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

		err = spill_expression(bb, reload_stack, expr);
		if (err)
			goto error_oom;
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
		.code = cu->method->jit_code,
	};
	int err = 0;

	buffer.buffer = cu->method->jit_code;
	buffer.pos = bb->start;

	if (bb->is_eh)
		stack_push(bb->mimic_stack, exception_ref_expr());

	while (buffer.pos < bb->end) {
		convert_fn_t convert;

		ctx.offset = ctx.buffer->pos;	/* this is fragile */
		ctx.opc = bytecode_read_u8(ctx.buffer);

		if (ctx.opc >= ARRAY_SIZE(converters)) {
			err = -EINVAL;
			break;
		}

		convert = converters[ctx.opc];
		if (!convert) {
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
	return err;
}
