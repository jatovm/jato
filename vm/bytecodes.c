/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains utility functions for parsing a bytecode stream.
 */

#include <vm/vm.h>
#include <vm/bytecode.h>
#include <vm/bytecodes.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

enum bytecode_type {
	BYTECODE_NORMAL		= 0x01,
	BYTECODE_BRANCH		= 0x02,
	BYTECODE_WIDE		= 0x04,
	BYTECODE_RETURN		= 0x08,
	BYTECODE_VARIABLE_LEN	= 0x10,
};

struct bytecode_info {
	unsigned char size;
	enum bytecode_type type;
};

#define BYTECODE(__opc, __name, __size, __type) \
	[__opc] = { .size = __size, .type = __type },

static struct bytecode_info bytecode_infos[] = {
#  include <vm/bytecode-def.h>
};

#undef BYTECODE

unsigned long bc_insn_size(unsigned char *bc_start)
{
	unsigned long size;

	size = bytecode_infos[*bc_start].size;
	if (*bc_start == OPC_WIDE)
		size += bytecode_infos[*++bc_start].size;

	if (size == 0) {
		printf("%s: Unknown bytecode opcode: 0x%x\n", __func__, *bc_start);
		abort();
	}
	return size;
}

bool bc_is_branch(unsigned char opc)
{
	return bytecode_infos[opc].type & BYTECODE_BRANCH;
}

bool bc_is_wide(unsigned char opc)
{
	return bytecode_infos[opc].type & BYTECODE_WIDE;
}

bool bc_is_goto(unsigned char opc)
{
	return opc == OPC_GOTO;
}

bool bc_is_athrow(unsigned char opc)
{
	return opc == OPC_ATHROW;
}

bool bc_is_return(unsigned char opc)
{
	return bytecode_infos[opc].type & BYTECODE_RETURN;
}

/**
 *	bc_target_off - Return branch opcode target offset.
 *	@code: start of branch bytecode.
 */
long bc_target_off(unsigned char *code)
{
	unsigned char opc = *code;

	if (bc_is_wide(opc))
		return read_s32(code + 1);

	return read_s16(code + 1);
}
