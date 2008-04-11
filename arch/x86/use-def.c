/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/compilation-unit.h>

enum {
	DEF_DST		= 1,
	DEF_SRC		= 2,
	DEF_NONE	= 4,
	USE_DST		= 8,
	USE_IDX_DST	= 16,	/* destination operand is memindex */
	USE_IDX_SRC	= 32,	/* source operand is memindex */
	USE_NONE	= 64,
	USE_SRC		= 128,
	USE_FP		= 256,	/* frame pointer */
};

struct insn_info {
	unsigned long flags;
};

#define DECLARE_INFO(_type, _flags) [_type] = { .flags = _flags }

static struct insn_info insn_infos[] = {
	DECLARE_INFO(INSN_ADD_IMM_REG, DEF_DST),
	DECLARE_INFO(INSN_ADD_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_AND_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_CALL_REG, USE_SRC | DEF_NONE),
	DECLARE_INFO(INSN_CALL_REL, USE_NONE | DEF_NONE),
	DECLARE_INFO(INSN_CLTD_REG_REG, USE_SRC | DEF_SRC | DEF_DST),
	DECLARE_INFO(INSN_CMP_IMM_REG, USE_DST),
	DECLARE_INFO(INSN_CMP_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_DIV_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_JE_BRANCH, USE_NONE | DEF_NONE),
	DECLARE_INFO(INSN_JNE_BRANCH, USE_NONE | DEF_NONE),
	DECLARE_INFO(INSN_JMP_BRANCH, USE_NONE | DEF_NONE),
	DECLARE_INFO(INSN_MOV_IMM_MEMBASE, USE_DST),
	DECLARE_INFO(INSN_MOV_IMM_REG, DEF_DST),
	DECLARE_INFO(INSN_MOV_MEMLOCAL_REG, USE_FP | DEF_DST),
	DECLARE_INFO(INSN_MOV_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_MOV_MEMINDEX_REG, USE_SRC | USE_IDX_SRC | DEF_DST),
	DECLARE_INFO(INSN_MOV_REG_MEMINDEX, USE_SRC | USE_DST | USE_IDX_DST | DEF_NONE),
	DECLARE_INFO(INSN_MOV_REG_MEMLOCAL, USE_SRC),
	DECLARE_INFO(INSN_MOV_REG_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_MUL_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_NEG_REG, USE_SRC | DEF_SRC),
	DECLARE_INFO(INSN_OR_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_PUSH_IMM, USE_NONE | DEF_NONE),
	DECLARE_INFO(INSN_PUSH_REG, USE_SRC | DEF_NONE),
	DECLARE_INFO(INSN_SAR_REG_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_SHL_REG_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_SHR_REG_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_SUB_MEMBASE_REG, USE_SRC | DEF_DST),
	DECLARE_INFO(INSN_XOR_MEMBASE_REG, USE_SRC | DEF_DST),
};

static inline struct insn_info *get_info(struct insn *insn)
{
	return insn_infos + insn->type;
}

bool insn_defs(struct insn *insn, unsigned long vreg)
{
	struct insn_info *info;

	info = get_info(insn);

	if (info->flags & DEF_SRC) {
		if (is_vreg(&insn->src.reg, vreg))
			return true;
	}

	if (info->flags & DEF_DST) {
		if (is_vreg(&insn->dest.reg, vreg))
			return true;
	}

	return false;
}

bool insn_uses(struct insn *insn, unsigned long vreg)
{
	struct insn_info *info;

	info = get_info(insn);

	if (info->flags & USE_SRC) {
		if (is_vreg(&insn->src.reg, vreg))
			return true;
	}

	if (info->flags & USE_DST) {
		if (is_vreg(&insn->dest.reg, vreg))
			return true;
	}

	if (info->flags & USE_IDX_SRC) {
		if (is_vreg(&insn->src.index_reg, vreg))
			return true;
	}

	if (info->flags & USE_IDX_DST) {
		if (is_vreg(&insn->dest.index_reg, vreg))
			return true;
	}

	return false;
}
