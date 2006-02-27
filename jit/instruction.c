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
			enum reg src_base_reg,
			unsigned long src_displacement,
			enum reg dest_reg)
{
	struct insn *insn = malloc(sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		insn->insn_op = insn_op;
		insn->src.base_reg = src_base_reg;
		insn->src.displacement = src_displacement;
		insn->dest.reg = dest_reg;
	}
	return insn;
}

void free_insn(struct insn *insn)
{
	free(insn);
}
