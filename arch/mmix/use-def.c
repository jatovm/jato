/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/compilation-unit.h"
#include "arch/instruction.h"

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

int insn_defs(struct compilation_unit *cu, struct insn *insn, struct var_info **defs)
{
	struct insn_info *info;
	int nr = 0;

	info = get_info(insn);

	if (info->flags & DEF_X)
		defs[nr++] = insn->x.reg.interval->var_info;

	return nr;
}

int insn_uses(struct insn *insn, struct var_info **uses)
{
	struct insn_info *info;
	int nr = 0;

	info = get_info(insn);

	if (info->flags & USE_Y)
		uses[nr++] = insn->y.reg.interval->var_info;

	if (info->flags & USE_Z)
		uses[nr++] = insn->z.reg.interval->var_info;

	return nr;
}
