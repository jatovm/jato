/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <compilation-unit.h>

#include <stdlib.h>
#include <string.h>

struct compilation_unit *alloc_compilation_unit(unsigned char *code,
						unsigned long code_len,
						struct basic_block *entry_bb)
{
	struct compilation_unit *cu = malloc(sizeof *cu);
	if (cu) {
		memset(cu, 0, sizeof *cu);
		INIT_LIST_HEAD(&cu->bb_list);
		if (entry_bb)
			list_add_tail(&entry_bb->bb_list_node, &cu->bb_list);
		cu->code = code;
		cu->code_len = code_len;
		cu->entry_bb = entry_bb;
		cu->expr_stack = alloc_stack();
	}
	return cu;
}

void free_compilation_unit(struct compilation_unit *cu)
{
	struct basic_block *bb, *tmp_bb;

	list_for_each_entry_safe(bb, tmp_bb, &cu->bb_list, bb_list_node)
		free_basic_block(bb);

	free_stack(cu->expr_stack);
	free(cu->objcode);
	free(cu);
}

/**
 * 	bb_find - Find basic block containing @offset.
 * 	@bb_list: First basic block in list.
 * 	@offset: Offset to find.
 * 
 * 	Find the basic block that contains the given offset and returns a
 * 	pointer to it.
 */
struct basic_block *find_bb(struct compilation_unit *cu, unsigned long offset)
{
	struct basic_block *bb;

	list_for_each_entry(bb, &cu->bb_list, bb_list_node) {
		if (offset >= bb->start && offset < bb->end)
			return bb;
	}
	return NULL;
}

unsigned long nr_bblocks(struct compilation_unit *cu)
{
	struct basic_block *bb;
	unsigned long nr = 0;

	list_for_each_entry(bb, &cu->bb_list, bb_list_node)
		nr++;

	return nr;
}
