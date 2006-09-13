/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/instruction.h>
#include <stdlib.h>
#include <string.h>

struct insn *alloc_insn(enum insn_type type)
{
	struct insn *insn = malloc(sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		INIT_LIST_HEAD(&insn->branch_list_node);
		insn->type = type;
	}
	return insn;
}

void free_insn(struct insn *insn)
{
	free(insn);
}

struct insn *insn(enum insn_opcode insn_op)
{
	return alloc_insn(DEFINE_INSN_TYPE_0(insn_op));
}

struct insn *membase_reg_insn(enum insn_opcode insn_op, enum reg src_base_reg,
			      long src_disp, enum reg dest_reg)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, OPERAND_MEMBASE, OPERAND_REGISTER));
	if (insn) {
		insn->src.base_reg = src_base_reg;
		insn->src.disp = src_disp;
		insn->dest.reg = dest_reg;
	}
	return insn;
}

struct insn *reg_membase_insn(enum insn_opcode insn_op, enum reg src_reg,
			      enum reg dest_base_reg, long dest_disp)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, OPERAND_REGISTER, OPERAND_MEMBASE));
	if (insn) {
		insn->src.reg = src_reg;
		insn->dest.base_reg = dest_base_reg;
		insn->dest.disp = dest_disp;
	}
	return insn;
}

struct insn *imm_reg_insn(enum insn_opcode insn_op, unsigned long imm,
			  enum reg reg)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, OPERAND_IMMEDIATE, OPERAND_REGISTER));
	if (insn) {
		insn->src.imm = imm;
		insn->dest.reg = reg;
	}
	return insn;
}

struct insn *imm_membase_insn(enum insn_opcode insn_op, unsigned long imm,
			   enum reg base_reg, long disp)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, OPERAND_IMMEDIATE, OPERAND_MEMBASE));
	if (insn) {
		insn->src.imm = imm;
		insn->dest.base_reg = base_reg;
		insn->dest.disp = disp;
	}
	return insn;
}

struct insn *reg_insn(enum insn_opcode insn_op, enum reg reg)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_1(insn_op, OPERAND_REGISTER));
	if (insn)
		insn->operand.reg = reg;

	return insn;
}

struct insn *reg_reg_insn(enum insn_opcode insn_op, enum reg src, enum reg dest)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_2(insn_op, OPERAND_REGISTER, OPERAND_REGISTER));
	if (insn) {
		insn->src.reg  = src;
		insn->dest.reg = dest;
	}
	return insn;
}

struct insn *imm_insn(enum insn_opcode insn_op, unsigned long imm)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_1(insn_op, OPERAND_IMMEDIATE));
	if (insn)
		insn->operand.imm = imm;

	return insn;
}

struct insn *rel_insn(enum insn_opcode insn_op, unsigned long rel)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_1(insn_op, OPERAND_RELATIVE));
	if (insn)
		insn->operand.rel = rel;

	return insn;
}

struct insn *branch_insn(enum insn_opcode insn_op, struct basic_block *if_true)
{
	struct insn *insn = alloc_insn(DEFINE_INSN_TYPE_1(insn_op, OPERAND_BRANCH));
	if (insn)
		insn->operand.branch_target = if_true;

	return insn;
}
