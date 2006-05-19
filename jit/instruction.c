/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <instruction.h>
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

struct insn *disp_reg_insn(enum insn_opcode insn_op, enum reg src_base_reg,
			   unsigned long src_disp, enum reg dest_reg)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, AM_DISP, AM_REG));
	if (insn) {
		insn->src.base_reg = src_base_reg;
		insn->src.disp = src_disp;
		insn->dest.reg = dest_reg;
	}
	return insn;
}

struct insn *imm_reg_insn(enum insn_opcode insn_op, unsigned long imm,
			  enum reg reg)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, AM_IMM, AM_REG));
	if (insn) {
		insn->src.imm = imm;
		insn->dest.reg = reg;
	}
	return insn;
}

struct insn *reg_insn(enum insn_opcode insn_op, enum reg reg)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE(insn_op, AM_REG));
	if (insn)
		insn->operand.reg = reg;

	return insn;
}

struct insn *imm_insn(enum insn_opcode insn_op, unsigned long imm)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE(insn_op, AM_IMM));
	if (insn)
		insn->operand.imm = imm;

	return insn;
}

struct insn *rel_insn(enum insn_opcode insn_op, unsigned long rel)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE(insn_op, AM_REL));
	if (insn)
		insn->operand.rel = rel;

	return insn;
}
