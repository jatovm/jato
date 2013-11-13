/*
 * Copyright (c) 2006,2009  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
#include "vm/bytecode.h"

#include "vm/opcodes.h"
#include "vm/class.h"
#include "vm/trace.h"
#include "vm/verifier.h"
#include "vm/die.h"
#include "vm/vm.h"

#include "jit/compiler.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

int8_t bytecode_read_s8(struct bytecode_buffer *buffer)
{
	return (int8_t) bytecode_read_u8(buffer);
}

int16_t bytecode_read_s16(struct bytecode_buffer *buffer)
{
	return (int16_t) bytecode_read_u16(buffer);
}

int32_t bytecode_read_s32(struct bytecode_buffer *buffer)
{
	return (int32_t) bytecode_read_u32(buffer);
}

uint8_t bytecode_read_u8(struct bytecode_buffer *buffer)
{
	return buffer->buffer[buffer->pos++];
}

uint16_t bytecode_read_u16(struct bytecode_buffer *buffer)
{
	uint16_t result;

	result  = (uint16_t) bytecode_read_u8(buffer) << 8;
	result |= (uint16_t) bytecode_read_u8(buffer);

	return result;
}

uint32_t bytecode_read_u32(struct bytecode_buffer *buffer)
{
	uint32_t result;

	result  = (uint32_t) bytecode_read_u8(buffer) << 24;
	result |= (uint32_t) bytecode_read_u8(buffer) << 16;
	result |= (uint32_t) bytecode_read_u8(buffer) << 8;
	result |= (uint32_t) bytecode_read_u8(buffer);

	return result;
}

int32_t bytecode_read_branch_target(unsigned char opc,
				    struct bytecode_buffer *buffer)
{
	if (bc_is_wide(opc))
		return bytecode_read_s32(buffer);

	return bytecode_read_s16(buffer);
}

uint8_t read_u8(const unsigned char *p)
{
	return *p;
}

int16_t read_s16(const unsigned char *p)
{
	return (int16_t) read_u16(p);
}

int32_t read_s32(const unsigned char *p)
{
	return (int32_t) read_u32(p);
}

uint16_t read_u16(const unsigned char *p)
{
	uint16_t result;

	result  = (uint16_t) read_u8(p + 0) << 8;
	result |= (uint16_t) read_u8(p + 1);

	return result;
}

uint32_t read_u32(const unsigned char *p)
{
	uint32_t result;

	result  = (uint32_t) read_u8(p + 0) << 24;
	result |= (uint32_t) read_u8(p + 1) << 16;
	result |= (uint32_t) read_u8(p + 2) << 8;
	result |= (uint32_t) read_u8(p + 3);

	return result;
}

void write_u8(unsigned char *p, uint8_t value)
{
	*p = value;
}

void write_u16(unsigned char *p, uint16_t value)
{
	write_u8(p + 0, (value >> 8) & 0xff);
	write_u8(p + 1, value & 0xff);
}

void write_u32(unsigned char *p, uint32_t value)
{
	write_u8(p + 0, (value >> 24) & 0xff);
	write_u8(p + 1, (value >> 16) & 0xff);
	write_u8(p + 2, (value >> 8) & 0xff);
	write_u8(p + 3, value & 0xff);
}

void write_s16(unsigned char *p, int16_t value)
{
	write_u16(p, (uint16_t) value);
}

void write_s32(unsigned char *p, int32_t value)
{
	write_u32(p, (uint32_t) value);
}

enum bytecode_type {
	BYTECODE_NORMAL		= 0x01,
	BYTECODE_BRANCH		= 0x02,
	BYTECODE_WIDE		= 0x04,
	BYTECODE_RETURN		= 0x08,
	BYTECODE_VARIABLE_LEN	= 0x10,
	BYTECODE_USES_LOCAL_VAR = 0x20,
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

unsigned int get_local_var_index(const unsigned char *code, unsigned long pc)
{
	uint8_t opc = code[pc];

	/* if the index is contained in the opcode */
	if (opc >= OPC_ILOAD_0 && opc < OPC_LLOAD_0)
		return opc - OPC_ILOAD_0;

	else if (opc >= OPC_LLOAD_0 && opc < OPC_FLOAD_0)
		return opc - OPC_LLOAD_0;

	else if (opc >= OPC_FLOAD_0 && opc < OPC_DLOAD_0)
		return opc - OPC_FLOAD_0;

	else if (opc >= OPC_DLOAD_0 && opc < OPC_ALOAD_0)
		return opc - OPC_DLOAD_0;

	else if (opc >= OPC_ALOAD_0 && opc < OPC_IALOAD)
		return opc - OPC_ALOAD_0;

	else if (opc >= OPC_ISTORE_0 && opc < OPC_LSTORE_0)
		return opc - OPC_ISTORE_0;

	else if (opc >= OPC_LSTORE_0 && opc < OPC_FSTORE_0)
		return opc - OPC_LSTORE_0;

	else if (opc >= OPC_FSTORE_0 && opc < OPC_DSTORE_0)
		return opc - OPC_FSTORE_0;

	else if (opc >= OPC_DSTORE_0 && opc < OPC_ASTORE_0)
		return opc - OPC_DSTORE_0;

	else if (opc >= OPC_ASTORE_0 && opc < OPC_IASTORE)
		return opc - OPC_ASTORE_0;

	/* if the index is the operand */

	return read_u8(code + pc + 1);
}

void get_tableswitch_info(const unsigned char *code, unsigned long pc,
			  struct tableswitch_info *info)
{
	unsigned long pad;

	assert(code[pc] == OPC_TABLESWITCH);

	pad = get_tableswitch_padding(pc);
	pc += pad + 1;

	info->default_target = read_u32(&code[pc]);
	info->low = read_u32(&code[pc + 4]);
	info->high = read_u32(&code[pc + 8]);
	info->targets = &code[pc + 12];

	info->count = info->high - info->low + 1;
	info->insn_size = 1 + pad + (3 + info->count) * 4;
}

void get_lookupswitch_info(const unsigned char *code, unsigned long pc,
			   struct lookupswitch_info *info)
{
	unsigned long pad;

	assert(code[pc] == OPC_LOOKUPSWITCH);

	pad = get_tableswitch_padding(pc);
	pc += pad + 1;

	info->default_target = read_u32(&code[pc]);
	info->count = read_u32(&code[pc + 4]);
	info->pairs = &code[pc + 8];

	info->insn_size = 1 + pad + 2 * 4 + info->count * 8;
}

unsigned long bc_insn_size(const unsigned char *code, unsigned long pc)
{
	unsigned long size;

	if (code[pc] == OPC_WIDE) {
		if (code[pc + 1] == OPC_IINC)
			return 6;

		return 4;
	}

	if (code[pc] == OPC_TABLESWITCH) {
		struct tableswitch_info info;

		get_tableswitch_info(code, pc, &info);
		return info.insn_size;
	}

	if (code[pc] == OPC_LOOKUPSWITCH) {
		struct lookupswitch_info info;

		get_lookupswitch_info(code, pc, &info);
		return info.insn_size;
	}

	size = bytecode_infos[code[pc]].size;

	if (size == 0)
		error("unknown bytecode opcode: 0x%x\n", code[pc]);

	return size;
}

/* Just like the function above but returns E_MALFORMED_BC if the code ends
 * in the middle of an instruction. */

long bc_insn_size_safe(unsigned char *code, unsigned long pc, unsigned long code_size)
{
	uint8_t opc;
	int high, low, padding, npairs, size;
	struct bytecode_buffer buf;

	buf.buffer = code;
	buf.pos = pc;

	if (pc >= code_size)
		return E_MALFORMED_BC;

	opc = bytecode_read_u8(&buf);

	if (bc_is_invalid(opc))
		return E_MALFORMED_BC;

	size = bytecode_infos[opc].size;
	if (size)
		goto out;

	if (opc == OPC_TABLESWITCH) {
		padding = get_tableswitch_padding(pc);

		if (buf.pos + padding + 12 >= code_size)
			return E_MALFORMED_BC;

		buf.pos += padding + 4; /* Ditch both the padding and the default */

		low = bytecode_read_s32(&buf);
		high = bytecode_read_s32(&buf);

		if (low > high)
			return E_MALFORMED_BC;

		size = 1 + padding + 12 + (high - low + 1) * 4;
	}
	else if (opc == OPC_LOOKUPSWITCH) {
		padding = get_tableswitch_padding(pc);

		if (buf.pos + padding + 8 >= code_size)
			return E_MALFORMED_BC;

		buf.pos += padding + 4; /* Ditch both the padding and the default */

		npairs = bytecode_read_s32(&buf);
		if (npairs < 0)
			return E_MALFORMED_BC;

		size = 1 + padding + 8 + npairs * 2 * 4;
	}
	else if (opc == OPC_WIDE) {
		if (buf.pos + 1 >= code_size)
			return E_MALFORMED_BC;

		opc = bytecode_read_u8(&buf);
		if (opc == OPC_IINC)
			size = 6;
		else
			size = 4;
	}
	else
		return E_MALFORMED_BC; /* unknown bytecode opcode. */

out:
	if (size < 0)
		return E_MALFORMED_BC;
	if ((unsigned long) pc + size > code_size)
		return E_MALFORMED_BC;

	return size;
}

bool bc_ends_basic_block(unsigned char code)
{
	return bc_is_branch(code) || bc_is_athrow(code) || bc_is_return(code);
}

bool bc_is_invalid(unsigned char opc)
{
	return opc >= OPC_BREAKPOINT
		|| opc == OPC_XXXUNUSEDXXX
		|| opc == OPC_XXXUNUSEDXXX_;
}

bool bc_is_ldc(unsigned char opc)
{
	return opc == OPC_LDC
		|| opc == OPC_LDC_W
		|| opc == OPC_LDC2_W;
}

bool bc_is_branch(unsigned char opc)
{
	return bytecode_infos[opc].type & BYTECODE_BRANCH;
}

bool bc_is_wide(unsigned char opc)
{
	return bytecode_infos[opc].type & BYTECODE_WIDE;
}

bool bc_uses_local_var(unsigned char opc)
{
	return bytecode_infos[opc].type & BYTECODE_USES_LOCAL_VAR;
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

bool bc_is_unconditionnal_branch(const unsigned char *code)
{
	return (bc_is_return(*code) || bc_is_goto(*code)
			|| bc_is_jsr(*code)
			|| bc_is_ret(code)
			|| bc_is_athrow(*code));
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

void bytecode_disassemble(struct vm_class *vmc, const unsigned char *code,
			  unsigned long size)
{
	unsigned long pc;

	bytecode_for_each_insn(code, size, pc) {
		char *opc_name;
		int size;
		int _pc;

		size = bc_insn_size(code, pc);

		trace_printf("   [ %-3ld ]  0x%02x  ", pc, code[pc]);

		opc_name = bc_get_insn_name(&code[pc]);
		if (!opc_name) {
			trace_printf("(string alloc failed)\n");
			continue;
		}

		_pc = pc;

		if (code[pc] == OPC_WIDE) {
			_pc++;
			size--;
		}

		if (size > 1)
			trace_printf("%-14s", opc_name);
		else
			trace_printf("%s", opc_name);

		free(opc_name);

		if (bc_is_branch(code[_pc]) && code[_pc] != OPC_TABLESWITCH) {
			trace_printf(" %ld\n", bc_target_off(&code[_pc]) + _pc);
			continue;
		}

		for (int i = 1; i < size; i++)
			trace_printf(" 0x%02x", (unsigned int)code[_pc + i]);

		if (code[_pc] == OPC_INVOKESPECIAL ||
		    code[_pc] == OPC_INVOKESTATIC ||
		    code[_pc] == OPC_INVOKEINTERFACE ||
		    code[_pc] == OPC_INVOKEVIRTUAL) {
			struct vm_class *r_vmc;
			char *r_name;
			char *r_type;
			uint16_t index;
			int err;

			index = read_u16(&code[_pc + 1]);

			if (code[_pc] == OPC_INVOKEINTERFACE)
				err = vm_class_resolve_interface_method(vmc,
					index, &r_vmc, &r_name, &r_type);
			else
				err = vm_class_resolve_method(vmc, index, &r_vmc,
						      &r_name, &r_type);

			if (!err)
				trace_printf(" // %s.%s%s", r_vmc->name, r_name,
					     r_type);
		}

		trace_printf("\n");
	}
}
