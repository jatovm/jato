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

struct insn *memlocal_reg_insn(enum insn_type insn_type,
			    struct stack_slot *src_slot,
			    struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.slot = src_slot;
		assoc_var_to_operand(dest_reg, insn, dest.reg);
	}
	return insn;
}

struct insn *membase_reg_insn(enum insn_type insn_type, struct var_info *src_base_reg,
			      long src_disp, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(src_base_reg, insn, src.base_reg);
		insn->src.disp = src_disp;
		assoc_var_to_operand(dest_reg, insn, dest.reg);
	}
	return insn;
}

struct insn *memindex_reg_insn(enum insn_type insn_type,
			       struct var_info *src_base_reg, struct var_info *src_index_reg,
			       unsigned char src_shift, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(src_base_reg, insn, src.base_reg);
		assoc_var_to_operand(src_index_reg, insn, src.index_reg);
		insn->src.shift = src_shift;
		assoc_var_to_operand(dest_reg, insn, dest.reg);
	}
	return insn;
}

struct insn *
reg_memlocal_insn(enum insn_type insn_type, struct var_info *src_reg,
		  struct stack_slot *dest_slot)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(src_reg, insn, src.reg);
		insn->dest.slot = dest_slot;
	}
	return insn;
}

struct insn *reg_memindex_insn(enum insn_type insn_type, struct var_info *src_reg,
			       struct var_info *dest_base_reg, struct var_info *dest_index_reg,
			       unsigned char dest_shift)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(src_reg, insn, src.reg);
		assoc_var_to_operand(dest_base_reg, insn, dest.base_reg);
		assoc_var_to_operand(dest_index_reg, insn, dest.index_reg);
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
		assoc_var_to_operand(result, insn, dest.reg);
	}
	return insn;
}

struct insn *imm_membase_insn(enum insn_type insn_type, unsigned long imm,
			      struct var_info *base_reg, long disp)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		insn->src.imm = imm;
		assoc_var_to_operand(base_reg, insn, dest.base_reg);
		insn->dest.disp = disp;
	}
	return insn;
}

struct insn *reg_insn(enum insn_type insn_type, struct var_info *reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(reg, insn, src.reg);
		assoc_var_to_operand(reg, insn, dest.reg);
	}

	return insn;
}

struct insn *reg_reg_insn(enum insn_type insn_type, struct var_info *src, struct var_info *dest)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		assoc_var_to_operand(src, insn, src.reg);
		assoc_var_to_operand(dest, insn, dest.reg);
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
