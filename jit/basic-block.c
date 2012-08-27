/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * This file contains functions for basic block operations.
 */

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/instruction.h"
#include "jit/statement.h"
#include "jit/ssa.h"

#include "vm/die.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "lib/buffer.h"

struct basic_block *do_alloc_basic_block(struct compilation_unit *b_parent, unsigned long start, unsigned long end)
{
	struct basic_block *bb;

	bb = malloc(sizeof(*bb));
	if (!bb)
		return NULL;
	memset(bb, 0, sizeof(*bb));

	bb->mimic_stack = alloc_stack();
	if (!bb->mimic_stack) {
		free(bb);
		return NULL;
	}
	INIT_LIST_HEAD(&bb->stmt_list);
	INIT_LIST_HEAD(&bb->insn_list);
	INIT_LIST_HEAD(&bb->bb_list_node);
	bb->b_parent = b_parent;
	bb->start = start;
	bb->end = end;
	bb->entry_mimic_stack_size = -1;

	return bb;
}

struct basic_block *alloc_basic_block(struct compilation_unit *b_parent, unsigned long start, unsigned long end)
{
	struct basic_block *ret;

	ret	= do_alloc_basic_block(b_parent, start, end);
	if (ret) {
		b_parent->nr_bb++;
	}
	return ret;
}

struct basic_block *get_basic_block(struct compilation_unit *cu, unsigned long start, unsigned long end)
{
	struct basic_block *bb;

	bb = alloc_basic_block(cu, start, end);
	if (bb)
		list_add_tail(&bb->bb_list_node, &cu->bb_list);
	return bb;
}

static void free_stmt_list(struct list_head *head)
{
	struct statement *stmt, *tmp;

	list_for_each_entry_safe(stmt, tmp, head, stmt_list_node)
		free_statement(stmt);
}

static void free_insn_list(struct list_head *head)
{
	struct insn *insn, *tmp;

	list_for_each_entry_safe(insn, tmp, head, insn_list_node)
		free_insn(insn);
}

void shrink_basic_block(struct basic_block *bb)
{
	free_stack(bb->mimic_stack);
	free_stmt_list(&bb->stmt_list);
	free_insn_list(&bb->insn_list);
	free(bb->resolution_blocks);
	free(bb->successors);
	free(bb->predecessors);
	free(bb->mimic_stack_expr);
	free(bb->dom_frontier);
	free(bb->use_set);
	free(bb->def_set);
	free(bb->live_in_set);
	free(bb->live_out_set);
}

void free_basic_block(struct basic_block *bb)
{
	free(bb);
}

/**
 *	bb_split - Split basic block into two.
 * 	@orig_bb: Basic block to split.
 * 	@offset: The end offset of the upper basic block and start offset
 *		of the bottom basic block.
 *
 * 	Splits the basic block into two parts and returns a pointer to the
 * 	newly allocated block. The end offset of the given basic block is
 * 	updated accordingly. Successors of the both block is also updated.
 */
struct basic_block *bb_split(struct basic_block *orig_bb, unsigned long offset)
{
	struct basic_block *new_bb;

	if (offset < orig_bb->start || offset > orig_bb->end)
		return NULL;

	new_bb = alloc_basic_block(orig_bb->b_parent, offset, orig_bb->end);
	if (!new_bb)
		return NULL;

	/* Insert new basic block after org_bb. */
	list_add(&new_bb->bb_list_node, &orig_bb->bb_list_node);

	orig_bb->end = offset;

	new_bb->entry_mimic_stack_size = -1;

	new_bb->successors = orig_bb->successors;
	orig_bb->successors = NULL;

	new_bb->nr_successors = orig_bb->nr_successors;
	orig_bb->nr_successors = 0;

	new_bb->predecessors = NULL;
	new_bb->nr_predecessors = 0;

	/* The original successors' predecessors must be updated to point to
	 * the new basic block. */
	for (unsigned int i = 0, n = new_bb->nr_successors; i < n; ++i) {
		struct basic_block *successor = new_bb->successors[i];

		for (unsigned int j = 0, m = successor->nr_predecessors;
			j < m; ++j)
		{
			if (successor->predecessors[j] == orig_bb)
				successor->predecessors[j] = new_bb;
		}
	}

	if (orig_bb->has_branch) {
		orig_bb->has_branch = false;
		new_bb->has_branch = true;

		new_bb->br_target_off = orig_bb->br_target_off;
		orig_bb->br_target_off = 0;
	}

	new_bb->has_return	= orig_bb->has_return;
	orig_bb->has_return	= false;

	return new_bb;
}

void bb_add_stmt(struct basic_block *bb, struct statement *stmt)
{
	list_add_tail(&stmt->stmt_list_node, &bb->stmt_list);
}

struct statement *bb_remove_last_stmt(struct basic_block *bb)
{
	struct list_head *last = list_last(&bb->stmt_list);

	list_del(last);

	return list_entry(last, struct statement, stmt_list_node);
}

void bb_add_insn(struct basic_block *bb, struct insn *insn)
{
	list_add_tail(&insn->insn_list_node, &bb->insn_list);
}

void bb_add_first_insn(struct basic_block *bb, struct insn *insn)
{
	list_add(&insn->insn_list_node, &bb->insn_list);
}

struct insn *bb_first_insn(struct basic_block *bb)
{
	return list_entry(bb->insn_list.next, struct insn, insn_list_node);
}

struct insn *bb_last_insn(struct basic_block *bb)
{
	return list_entry(bb->insn_list.prev, struct insn, insn_list_node);
}

/*
 * Move part of the contents from one basic block (from basic_block)
 * to another (to basic block).
 */
static void ssa_bb_move(struct basic_block *from, struct basic_block *to)
{
	struct insn *this, *next;

	list_for_each_entry_safe(this, next, &from->insn_list, insn_list_node) {
		list_move(&this->insn_list_node, to->insn_list.prev);
	}

	to->b_parent = from->b_parent;
	to->start = from->start;
	to->end = from->end;

	to->nr_successors = from->nr_successors;
	to->successors = from->successors;

	to->nr_predecessors = from->nr_predecessors;
	to->predecessors = from->predecessors;

	to->dom_frontier = from->dom_frontier;
	from->dom_frontier = NULL;
	to->nr_predecessors_no_eh = from->nr_predecessors_no_eh;
	to->positions_as_predecessor = from->positions_as_predecessor;
	from->positions_as_predecessor = NULL;
	to->dom_successors = from->dom_successors;
	from->dom_successors = NULL;
	to->nr_dom_successors = from->nr_dom_successors;

	to->dfn = from->dfn;
	from->dfn = 0;
}

static int __bb_add_neighbor(void *new, void **array, unsigned long *nb)
{
	unsigned long new_size;
	void *new_array;

	new_size = sizeof(void *) * (*nb + 1);

	new_array = realloc(*array, new_size);
	if (new_array == NULL)
		return warn("out of memory"), -ENOMEM;

	*array = new_array;

	((void **)(*array))[*nb] = new;
	(*nb)++;

	return 0;
}

int bb_add_successor(struct basic_block *bb, struct basic_block *successor)
{
	__bb_add_neighbor(bb, (void **)&successor->predecessors, &successor->nr_predecessors);
	return __bb_add_neighbor(successor, (void **)&bb->successors, &bb->nr_successors);
}

#if 0
int bb_add_predecessor(struct basic_block *bb, struct basic_block *predecessor)
{
	return __bb_add_neighbor(predecessor, (void **)&bb->predecessors, &bb->nr_predecessors);
}
#endif

/*
 * This function inserts a new basic block after bb and moves
 * the contents of bb into the new basic block.
 */
struct basic_block *ssa_insert_chg_bb(struct compilation_unit *cu,
		struct basic_block *pred_bb,
		struct basic_block *bb,
		unsigned int bc_offset)
{
	struct basic_block *new_bb;

	new_bb = alloc_basic_block(cu, bc_offset, bc_offset);
	if (!new_bb)
		return NULL;

	ssa_bb_move(bb, new_bb);

	bb->end = bb->start;

	list_add(&new_bb->bb_list_node, &bb->bb_list_node);

	bb->nr_predecessors = 1;
	bb->predecessors = malloc(sizeof(struct basic_block *));
	if (!bb->predecessors)
		return NULL;
	bb->predecessors[0] = pred_bb;

	bb->nr_successors = 1;
	bb->successors = malloc(sizeof(struct basic_block *));
	if (!bb->successors)
		return NULL;
	bb->successors[0] = new_bb;

	for(unsigned long i = 0; i < new_bb->nr_predecessors; i++)
		if (new_bb->predecessors[i] == pred_bb) {
			new_bb->predecessors[i] = bb;
			break;
		}

	/*
	 * All references to bb have to be modified so that they point to new_bb
	 * (this includes successors basic block list, predecessors basic block
	 * list and branch targets in instructions).
	 */
	for (unsigned long i = 0; i < new_bb->nr_predecessors; i++) {
		struct basic_block *bb_pred = new_bb->predecessors[i];
		struct insn *last;

		for (unsigned long j = 0; j < bb_pred->nr_successors; j++)
			if (bb_pred->successors[j] == bb) {
				bb_pred->successors[j] = new_bb;
				break;
			}

		last = bb_last_insn(bb_pred);
		if (last && insn_is_branch(last))
			if (last->operand.branch_target && last->operand.branch_target == bb)
				last->operand.branch_target = new_bb;

	}

	for (unsigned long i = 0; i < new_bb->nr_successors; i++) {
		struct basic_block *bb_succ = new_bb->successors[i];

		for(unsigned long j = 0; j < bb_succ->nr_predecessors; j++)
			if (bb_succ->predecessors[j] == bb) {
				bb_succ->predecessors[j] = new_bb;

				break;
			}
	}

	return new_bb;

}

/*
 * This function inserts an empty basic block between
 * pred_bb and bb, where pred_bb is a predecessor basic block
 * of bb.
 */
struct basic_block *ssa_insert_empty_bb(struct compilation_unit *cu,
				struct basic_block *pred_bb,
				struct basic_block *bb,
				unsigned int bc_offset)
{
	struct basic_block *new_bb;

	new_bb = alloc_basic_block(cu, bc_offset, bc_offset);
	if (!new_bb)
		return NULL;

	list_add(&new_bb->bb_list_node, &pred_bb->bb_list_node);

	for (unsigned long i = 0; i < pred_bb->nr_successors; i++) {
		if (pred_bb->successors[i] == bb) {
			pred_bb->successors[i] = new_bb;
			break;
		}
	}

	new_bb->nr_predecessors = 1;
	new_bb->predecessors = malloc(sizeof(struct basic_block *));
	if (!new_bb->predecessors)
		return NULL;
	new_bb->predecessors[0] = pred_bb;

	for (unsigned long i = 0; i < bb->nr_predecessors; i++) {
		if (bb->predecessors[i] == pred_bb) {
			bb->predecessors[i] = new_bb;
			break;
		}
	}

	new_bb->nr_successors = 1;
	new_bb->successors = malloc(sizeof(struct basic_block *));
	if (!new_bb->successors)
		return NULL;
	new_bb->successors[0] = bb;

	return new_bb;
}

bool bb_successors_contains(struct basic_block *bb, struct basic_block *lookup_bb)
{
	for (unsigned long i = 0; i < bb->nr_successors; i++)
		if (bb->successors[i] == lookup_bb)
			return true;

	return false;
}

int bb_add_mimic_stack_expr(struct basic_block *bb, struct expression *expr)
{
	return __bb_add_neighbor(expr, (void **)&bb->mimic_stack_expr, &bb->nr_mimic_stack_expr);
}

unsigned char *bb_native_ptr(struct basic_block *bb)
{
	return buffer_ptr(bb->b_parent->objcode) + bb->mach_offset;
}

void resolution_block_init(struct resolution_block *block)
{
	INIT_LIST_HEAD(&block->insns);
}

int bb_lookup_successor_index(struct basic_block *from, struct basic_block *to)
{
	for (unsigned long i = 0; i < from->nr_successors; i++)
		if (from->successors[i] == to)
			return i;

	return -1;
}

bool branch_needs_resolution_block(struct basic_block *from, int idx)
{
	return !list_is_empty(&from->resolution_blocks[idx].insns);
}

void clear_mimic_stack(struct stack *stack)
{
	struct expression *expr;

	while (!stack_is_empty(stack)) {
		expr = stack_pop(stack);
		expr_put(expr);
	}
}
