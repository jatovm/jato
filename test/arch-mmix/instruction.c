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

static void init_none_operand(struct insn *insn, unsigned long idx)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_NONE;
}

static void init_branch_operand(struct insn *insn, unsigned long idx, struct basic_block *if_true)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_BRANCH;
	operand->branch_target = if_true;
}

static void init_imm_operand(struct insn *insn, unsigned long idx, unsigned long imm)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_IMM;
	operand->imm = imm;
}

static void init_local_operand(struct insn *insn, unsigned long idx, struct stack_slot *slot)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_LOCAL;
	operand->slot = slot;
}

static void init_reg_operand(struct insn *insn, unsigned long idx, struct var_info *reg)
{
	struct operand *operand = &insn->operands[idx];

	operand->type = OPERAND_REG;
	init_register(&operand->reg, insn, reg->interval);
}

struct insn *imm_insn(enum insn_type insn_type, unsigned long imm, struct var_info *result)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, result);
		init_imm_operand(insn, 1, imm);
		/* part of the immediate is stored in z but it's unused here */
		init_none_operand(insn, 2);
	}
	return insn;
}

struct insn *arithmetic_insn(enum insn_type insn_type, struct var_info *z, struct var_info *y, struct var_info *x)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, x);
		init_reg_operand(insn, 1, y);
		init_reg_operand(insn, 2, z);
	}
	return insn;
}

struct insn *branch_insn(enum insn_type insn_type, struct basic_block *if_true)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_branch_operand(insn, 0, if_true);
		init_none_operand(insn, 1);
		init_none_operand(insn, 2);
	}
	return insn;
}

struct insn *st_insn(enum insn_type insn_type, struct var_info *var, struct stack_slot *slot)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, var);
		init_local_operand(insn, 1, slot);
		init_none_operand(insn, 2);
	}
	return insn;
}

struct insn *ld_insn(enum insn_type insn_type, struct stack_slot *slot, struct var_info *var)
{
	struct insn *insn = alloc_insn(insn_type);
	if (insn) {
		init_reg_operand(insn, 0, var);
		init_local_operand(insn, 1, slot);
		init_none_operand(insn, 2);
	}
	return insn;
}
