/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/list.h>

#include <arch/instruction.h>

#include <stdlib.h>
#include <string.h>

struct insn *alloc_insn(enum insn_type type)
{
	struct insn *insn = malloc(sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		insn->type = type;
	}
	return insn;
}

void free_insn(struct insn *insn)
{
	free(insn);
}

struct insn *imm_insn(enum insn_type insn_type, unsigned long imm, struct var_info *result)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(result, insn, x.reg);
		insn->y.imm = imm;
		/* part of the immediate is stored in z but it's unused here */
	}
	return insn;
}

struct insn *arithmetic_insn(enum insn_type insn_type, struct var_info *z, struct var_info *y, struct var_info *x)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(x, insn, x.reg);
		assoc_var_to_operand(y, insn, y.reg);
		assoc_var_to_operand(z, insn, z.reg);
	}
	return insn;
}

struct insn *branch_insn(enum insn_type insn_type, struct basic_block *if_true)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn)
		insn->operand.branch_target = if_true;

	return insn;
}
