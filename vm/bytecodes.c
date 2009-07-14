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

#include <assert.h>
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

	if (*bc_start == OPC_WIDE) {
		if (*(bc_start + 1) == OPC_IINC)
			return 6;

		return 4;
	}

	size = bytecode_infos[*bc_start].size;

	if (size == 0)
		error("unknown bytecode opcode: 0x%x\n", *bc_start);

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

bool bc_is_jsr(unsigned char opc)
{
	return opc == OPC_JSR || opc == OPC_JSR_W;
}

bool bc_is_ret(const unsigned char *code)
{
	if (*code == OPC_WIDE)
		code++;

	return *code == OPC_RET;
}

bool bc_is_astore(const unsigned char *code)
{
	if (*code == OPC_WIDE)
		code++;

	switch (*code) {
	case OPC_ASTORE:
	case OPC_ASTORE_0:
	case OPC_ASTORE_1:
	case OPC_ASTORE_2:
	case OPC_ASTORE_3:
		return true;
	default:
		return false;
	}
}

unsigned long bc_get_astore_index(const unsigned char *code)
{
	if (*code == OPC_WIDE)
		return read_u16(code + 2);

	switch (*code) {
	case OPC_ASTORE:
		return read_u8(code + 1);
	case OPC_ASTORE_0:
		return 0;
	case OPC_ASTORE_1:
		return 1;
	case OPC_ASTORE_2:
		return 2;
	case OPC_ASTORE_3:
		return 3;
	default:
		assert(!"not an astore instruction");
	}
}

unsigned long bc_get_ret_index(const unsigned char *code)
{
	if (*code == OPC_WIDE)
		return read_u16(code + 2);

	return read_u8(code + 1);
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

/**
 *	bc_set_target_off - Sets opcode target offset. The width of instruction
 *	is untouched. It's the caller's responsibility to assure that
 *	offset is within range.
 *	@code: start of branch bytecode.
 */
void bc_set_target_off(unsigned char *code, long off)
{
	if (bc_is_wide(*code)) {
		assert(off <= 0x7ffffffe && off >= -0x7fffffff);
		write_s32(code + 1, (int32_t)off);
	} else {
		assert(off <= 0x7ffe && off >= -0x7fff);
		write_s16(code + 1, (int16_t)off);
	}
}

static char *bc_get_insn_name(const unsigned char *code)
{
	char buf[16];
	int buf_index;

	buf_index = 0;

	if (*code == OPC_WIDE) {
		strcpy(buf, "wide ");
		buf_index = 5;
		code++;
	}

	const char *opc_name = bytecode_infos[*code].name + 4;

	while (*opc_name)
		buf[buf_index++] = tolower(*opc_name++);

	buf[buf_index] = 0;

	return strdup(buf);
}

void bytecode_disassemble(const unsigned char *code, unsigned long size)
{
	unsigned long pc;

	bytecode_for_each_insn(code, size, pc) {
		char *opc_name;
		int size;

		size = bc_insn_size(&code[pc]);

		printf("   [ %-3ld ]  0x%02x  ", pc, code[pc]);

		if (code[pc] == OPC_WIDE)
			size--;

		opc_name = bc_get_insn_name(&code[pc]);
		if (!opc_name) {
			printf("(string alloc failed)\n");
			continue;
		}

		if (size > 1)
			printf("%-14s", opc_name);
		else
			printf("%s", opc_name);

		free(opc_name);

		if (bc_is_branch(code[pc])) {
			printf(" %ld\n", bc_target_off(&code[pc]) + pc);
			continue;
		}

		for (int i = 1; i < size; i++)
			printf(" 0x%02x", (unsigned int)code[pc + i]);

		printf("\n");
	}
}
