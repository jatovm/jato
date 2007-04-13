/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <arch/instruction.h>

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

struct insn *insn(enum insn_type insn_type)
{
	return alloc_insn(insn_type);
}

struct insn *membase_reg_insn(enum insn_type insn_type, struct var_info *src_base_reg,
			      long src_disp, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.base_reg = src_base_reg;
		insn->src.disp = src_disp;
		insn->dest.reg = dest_reg;
	}
	return insn;
}

struct insn *memindex_reg_insn(enum insn_type insn_type,
			       struct var_info *src_base_reg, struct var_info *src_index_reg,
			       unsigned char src_shift, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.base_reg = src_base_reg;
		insn->src.index_reg = src_index_reg;
		insn->src.shift = src_shift;
		insn->dest.reg = dest_reg;
	}
	return insn;
}

struct insn *reg_membase_insn(enum insn_type insn_type, struct var_info *src_reg,
			      struct var_info *dest_base_reg, long dest_disp)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.reg = src_reg;
		insn->dest.base_reg = dest_base_reg;
		insn->dest.disp = dest_disp;
	}
	return insn;
}

struct insn *reg_memindex_insn(enum insn_type insn_type, struct var_info *src_reg,
			       struct var_info *dest_base_reg, struct var_info *dest_index_reg,
			       unsigned char dest_shift)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.reg = src_reg;
		insn->dest.base_reg = dest_base_reg;
		insn->dest.index_reg = dest_index_reg;
		insn->dest.shift = dest_shift;
	}
	return insn;
}

struct insn *imm_reg_insn(enum insn_type insn_type, unsigned long imm,
			  struct var_info *result)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.imm = imm;
		insn->dest.reg = result;
	}
	return insn;
}

struct insn *imm_membase_insn(enum insn_type insn_type, unsigned long imm,
			      struct var_info *base_reg, long disp)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.imm = imm;
		insn->dest.base_reg = base_reg;
		insn->dest.disp = disp;
	}
	return insn;
}

struct insn *reg_insn(enum insn_type insn_type, struct var_info *reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.reg = reg;
		insn->dest.reg = reg;
	}

	return insn;
}

struct insn *reg_reg_insn(enum insn_type insn_type, struct var_info *src, struct var_info *dest)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.reg  = src;
		insn->dest.reg = dest;
	}
	return insn;
}

struct insn *imm_insn(enum insn_type insn_type, unsigned long imm)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn)
		insn->operand.imm = imm;

	return insn;
}

struct insn *rel_insn(enum insn_type insn_type, unsigned long rel)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn)
		insn->operand.rel = rel;

	return insn;
}

struct insn *branch_insn(enum insn_type insn_type, struct basic_block *if_true)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn)
		insn->operand.branch_target = if_true;

	return insn;
}
