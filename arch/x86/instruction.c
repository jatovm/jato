/*
 * Copyright (c) 2006-2008  Pekka Enberg
 * 
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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

static bool operand_uses_reg(struct operand *operand)
{
	return operand->type == OPERAND_REG
		|| operand->type == OPERAND_MEMBASE
		|| operand->type == OPERAND_MEMINDEX;
}

static bool operand_uses_index_reg(struct operand *operand)
{
	return operand->type == OPERAND_MEMINDEX;
}

static void release_operand(struct operand *operand)
{
	if (operand_uses_reg(operand))
		list_del(&operand->reg.use_pos_list);

	if (operand_uses_index_reg(operand))
		list_del(&operand->index_reg.use_pos_list);
}

void free_insn(struct insn *insn)
{
	release_operand(&insn->operands[0]);
	release_operand(&insn->operands[1]);

	free(insn);
}

static void init_none_operand(struct insn *insn, unsigned long idx)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_NONE;
}

static void init_branch_operand(struct insn *insn, unsigned long idx,
				struct basic_block *if_true)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_BRANCH;
	operand->branch_target = if_true;
}

static void init_imm_operand(struct insn *insn, unsigned long idx,
			     unsigned long imm)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_IMM;
	operand->imm = imm;
}

static void init_membase_operand(struct insn *insn, unsigned long idx,
				 struct var_info *base_reg, unsigned long disp)
{
	struct operand *operand;

	operand = &insn->operands[idx];
	operand->type = OPERAND_MEMBASE;
	operand->disp = disp;

	init_register(&operand->base_reg, insn, base_reg->interval);
}

static void init_memindex_operand(struct insn *insn, unsigned long idx,
				  struct var_info *base_reg,
				  struct var_info *index_reg, unsigned long shift)
{
	struct operand *operand;

	operand = &insn->operands[idx];
	operand->type = OPERAND_MEMINDEX;
	operand->shift = shift;

	init_register(&operand->base_reg, insn, base_reg->interval);
	init_register(&operand->index_reg, insn, index_reg->interval);
}

static void init_memlocal_operand(struct insn *insn, unsigned long idx,
				  struct stack_slot *slot)
{
	struct operand *operand;

	operand = &insn->operands[idx];
	operand->type = OPERAND_MEMLOCAL;
	operand->slot = slot;
}

static void init_reg_operand(struct insn *insn, unsigned long idx,
			     struct var_info *reg)
{
	struct operand *operand;

	operand = &insn->operands[idx];
	operand->type = OPERAND_REG;

	init_register(&operand->reg, insn, reg->interval);
}

static void init_rel_operand(struct insn *insn, unsigned long idx,
			     unsigned long rel)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_REL;
	operand->rel = rel;
}

struct insn *insn(enum insn_type insn_type)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_none_operand(insn, 0);
		init_none_operand(insn, 1);
	}
	return insn;
}

struct insn *memlocal_reg_insn(enum insn_type insn_type,
			    struct stack_slot *src_slot,
			    struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_memlocal_operand(insn, 0, src_slot);
		init_reg_operand(insn, 1, dest_reg);
	}
	return insn;
}

struct insn *membase_reg_insn(enum insn_type insn_type, struct var_info *src_base_reg,
			      long src_disp, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_membase_operand(insn, 0, src_base_reg, src_disp);
		init_reg_operand(insn, 1, dest_reg);
	}
	return insn;
}

struct insn *memindex_reg_insn(enum insn_type insn_type,
			       struct var_info *src_base_reg, struct var_info *src_index_reg,
			       unsigned char src_shift, struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_memindex_operand(insn, 0, src_base_reg, src_index_reg, src_shift);
		init_reg_operand(insn, 1, dest_reg);
	}
	return insn;
}

struct insn *
reg_membase_insn(enum insn_type insn_type, struct var_info *src_reg,
		 struct var_info *dest_base_reg, long dest_disp)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, src_reg);
		init_membase_operand(insn, 1, dest_base_reg, dest_disp);
	}
	return insn;
}

struct insn *
reg_memlocal_insn(enum insn_type insn_type, struct var_info *src_reg,
		  struct stack_slot *dest_slot)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, src_reg);
		init_memlocal_operand(insn, 1, dest_slot);
	}
	return insn;
}

struct insn *reg_memindex_insn(enum insn_type insn_type, struct var_info *src_reg,
			       struct var_info *dest_base_reg, struct var_info *dest_index_reg,
			       unsigned char dest_shift)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, src_reg);
		init_memindex_operand(insn, 1, dest_base_reg, dest_index_reg, dest_shift);
	}
	return insn;
}

struct insn *imm_reg_insn(enum insn_type insn_type, unsigned long imm,
			  struct var_info *dest_reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_imm_operand(insn, 0, imm);
		init_reg_operand(insn, 1, dest_reg);
	}
	return insn;
}

struct insn *imm_membase_insn(enum insn_type insn_type, unsigned long imm,
			      struct var_info *base_reg, long disp)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_imm_operand(insn, 0, imm);
		init_membase_operand(insn, 1, base_reg, disp);
	}
	return insn;
}

struct insn *reg_insn(enum insn_type insn_type, struct var_info *reg)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, reg);
		init_none_operand(insn, 1);
	}

	return insn;
}

struct insn *reg_reg_insn(enum insn_type insn_type, struct var_info *src, struct var_info *dest)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, src);
		init_reg_operand(insn, 1, dest);
	}
	return insn;
}

struct insn *imm_insn(enum insn_type insn_type, unsigned long imm)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_imm_operand(insn, 0, imm);
		init_none_operand(insn, 1);
	}
	return insn;
}

struct insn *rel_insn(enum insn_type insn_type, unsigned long rel)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_rel_operand(insn, 0, rel);
		init_none_operand(insn, 1);
	}
	return insn;
}

struct insn *branch_insn(enum insn_type insn_type, struct basic_block *if_true)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_branch_operand(insn, 0, if_true);
		init_none_operand(insn, 1);
	}
	return insn;
}
