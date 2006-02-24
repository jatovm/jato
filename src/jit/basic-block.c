/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 * 
 * This file contains functions for basic block operations.
 */

#include <basic-block.h>
#include <instruction.h>
#include <statement.h>
#include <stdlib.h>
#include <string.h>

struct basic_block *alloc_basic_block(unsigned long start, unsigned long end)
{
	struct basic_block *bb = malloc(sizeof(*bb));
	if (bb) {
		memset(bb, 0, sizeof(*bb));
		INIT_LIST_HEAD(&bb->insn_list);
		bb->label_stmt = alloc_statement(STMT_LABEL);
		bb->start = start;
		bb->end = end;
		bb->next = NULL;
	}
	return bb;
}

void free_basic_block(struct basic_block *bb)
{
	struct insn *insn, *tmp;

	list_for_each_entry_safe(insn, tmp, &bb->insn_list, insns)
		free_insn(insn);

	free_statement(bb->label_stmt);
	free_statement(bb->stmt);
	free(bb);
}

/**
 *	bb_split - Split basic block into two.
 * 	@bb: Basic block to split.
 * 	@offset: The end offset of the upper basic block and start offset
 *		of the bottom basic block.
 *
 * 	Splits the basic block into two parts and returns a pointer to the
 * 	newly allocated block. The end offset of the given basic block is
 * 	updated accordingly.
 */
struct basic_block *bb_split(struct basic_block *bb, unsigned long offset)
{
	struct basic_block *new_block;

	if (offset < bb->start || offset >= bb->end)
		return NULL;

	new_block = alloc_basic_block(offset, bb->end);
	if (new_block) {
		new_block->next = bb->next;
		bb->next = new_block;
		bb->end = offset;
	}
	return new_block;
}

/**
 * 	bb_find - Find basic block containing @offset.
 * 	@bb_list: First basic block in list.
 * 	@offset: Offset to find.
 * 
 * 	Find the basic block that contains the given offset and returns a
 * 	pointer to it.
 */
struct basic_block *bb_find(struct basic_block *bb_list, unsigned long offset)
{
	struct basic_block *bb = bb_list;

	while (bb) {
		if (offset >= bb->start && offset < bb->end)
			break;
		bb = bb->next;
	}
	return bb;
}

unsigned long nr_bblocks(struct basic_block *entry_bb)
{
	unsigned long ret = 0;
	struct basic_block *bb = entry_bb;
	while (bb) {
		ret++;
		bb = bb->next;
	}

	return ret;
}

void bb_insert_insn(struct basic_block *bb, struct insn *insn)
{
	list_add_tail(&insn->insns, &bb->insn_list);
}
