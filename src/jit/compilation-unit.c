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
						struct basic_block *entry_bb,
						struct stack *expr_stack)
{
	struct compilation_unit *cu = malloc(sizeof *cu);
	if (cu) {
		memset(cu, 0, sizeof *cu);
		cu->code = code;
		cu->code_len = code_len;
		cu->entry_bb = entry_bb;
		cu->expr_stack = expr_stack;
	}
	return cu;
}

void free_compilation_unit(struct compilation_unit *cu)
{
	struct basic_block *bb = cu->entry_bb;

	while (bb) {
		struct basic_block *prev = bb;
		bb = bb->next;
		free_basic_block(prev);
	}
	free(cu);
}
