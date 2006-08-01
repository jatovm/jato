/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 * 
 * This file contains functions for basic block operations.
 */

#include <jit/basic-block.h>
#include <jit/compilation-unit.h>
#include <jit/instruction.h>
#include <jit/statement.h>
#include <stdlib.h>
#include <string.h>

struct basic_block *alloc_basic_block(struct compilation_unit *b_parent, unsigned long start, unsigned long end)
{
	struct basic_block *bb = malloc(sizeof(*bb));
	if (bb) {
		memset(bb, 0, sizeof(*bb));
		INIT_LIST_HEAD(&bb->stmt_list);
		INIT_LIST_HEAD(&bb->insn_list);
		INIT_LIST_HEAD(&bb->backpatch_insns);
		INIT_LIST_HEAD(&bb->bb_list_node);
		bb->b_parent = b_parent;
		bb->start = start;
		bb->end = end;
	}
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

void free_basic_block(struct basic_block *bb)
{
	free_stmt_list(&bb->stmt_list);
	free_insn_list(&bb->insn_list);
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
 * 	updated accordingly.
 */
struct basic_block *bb_split(struct basic_block *orig_bb, unsigned long offset)
{
	struct basic_block *new_bb;

	if (offset < orig_bb->start || offset >= orig_bb->end)
		return NULL;

	new_bb = alloc_basic_block(orig_bb->b_parent, offset, orig_bb->end);
	if (new_bb)
		orig_bb->end = offset;
	return new_bb;
}

void bb_add_stmt(struct basic_block *bb, struct statement *stmt)
{
	list_add_tail(&stmt->stmt_list_node, &bb->stmt_list);
}

void bb_add_insn(struct basic_block *bb, struct insn *insn)
{
	list_add_tail(&insn->insn_list_node, &bb->insn_list);
}
