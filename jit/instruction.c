/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <instruction.h>
#include <stdlib.h>
#include <string.h>

struct insn *alloc_insn(enum insn_opcode insn_op,
			unsigned long src, unsigned long dest)
{
	struct insn *insn = malloc(sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		insn->insn_op = insn_op;
		insn->src = src;
		insn->dest = dest;
	}
	return insn;
}

void free_insn(struct insn *insn)
{
	free(insn);
}
