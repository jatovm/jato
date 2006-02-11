/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <basic-block.h>
#include <stdlib.h>

struct basic_block *alloc_basic_block(unsigned long start, unsigned long end)
{
	struct basic_block *bb = malloc(sizeof(*bb));
	if (bb) {
		bb->start = start;
		bb->end = end;
		bb->next = NULL;
	}
	return bb;
}

void free_basic_block(struct basic_block *bb)
{
	free(bb);
}

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
