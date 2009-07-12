/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains utility functions for parsing a bytecode stream.
 */

#include "vm/vm.h"
#include "vm/bytecode.h"
#include "vm/bytecodes.h"
#include "vm/die.h"
#include "vm/opcodes.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
	const char *name;
};

#define BYTECODE(__opc, __name, __size, __type) \
	[__opc] = { .size = __size, .name = #__opc, .type = __type },

static struct bytecode_info bytecode_infos[] = {
#  include <vm/bytecode-def.h>
};

#undef BYTECODE

unsigned long bc_insn_size(const unsigned char *bc_start)
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
long bc_target_off(const unsigned char *code)
{
	unsigned char opc = *code;

	if (bc_is_wide(opc))
		return read_s32(code + 1);

	return read_s16(code + 1);
}

void bytecode_disassemble(const unsigned char *code, unsigned long size)
{
	unsigned long pc;

	bytecode_for_each_insn(code, size, pc) {
		const char *opc_name;
		char tmp_name[16];
		int size;
		int i;

		opc_name = bytecode_infos[code[pc]].name + 4;
		size = bc_insn_size(&code[pc]);

		for (i = 0; *opc_name; opc_name++, i++)
			tmp_name[i] = tolower(*opc_name);

		tmp_name[i] = 0;

		printf("   [ %-3ld ]  0x%02x  ", pc, code[pc]);

		if (size > 1)
			printf("%-14s", tmp_name);
		else
			printf("%s", tmp_name);

		if (bc_is_branch(code[pc])) {
			printf(" %ld\n", bc_target_off(&code[pc]) + pc);
			continue;
		}

		for (int i = 1; i < size; i++)
			printf(" 0x%02x", (unsigned int)code[pc + i]);

		printf("\n");
	}
}
