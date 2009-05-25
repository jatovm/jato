/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/compilation-unit.h>
#include <arch/instruction.h>

enum {
	NO_USE_DEF	= 0,
	DEF_X		= 1,
	USE_Y		= 4,
	USE_Z		= 8,
};

struct insn_info {
	unsigned char flags;
};

#define DECLARE_INFO(_type, _flags) [_type] = { .flags = _flags }

static struct insn_info insn_infos[] = {
	DECLARE_INFO(INSN_ADD, DEF_X | USE_Y | USE_Z),
	DECLARE_INFO(INSN_JMP, NO_USE_DEF),
	DECLARE_INFO(INSN_SETL, DEF_X),
};

static inline struct insn_info *get_info(struct insn *insn)
{
	return insn_infos + insn->type;
}

bool insn_defs(struct insn *insn, struct var_info *var)
{
	struct insn_info *info;
	unsigned long vreg;

	info = get_info(insn);
	vreg = var->vreg;

	if (info->flags & DEF_X) {
		if (is_vreg(&insn->x.reg, vreg))
			return true;
	}
	return false;
}

bool insn_uses(struct insn *insn, struct var_info *var)
{
	struct insn_info *info;
	unsigned long vreg;

	info = get_info(insn);
	vreg = var->vreg;

	if (info->flags & USE_Y) {
		if (is_vreg(&insn->y.reg, vreg))
			return true;
	}
	if (info->flags & USE_Z) {
		if (is_vreg(&insn->z.reg, vreg))
			return true;
	}
	return false;
}
