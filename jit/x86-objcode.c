/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for emitting IA-32 object code from IR
 * instruction sequence.
 */

#include <errno.h>
#include <x86-objcode.h>
#include <basic-block.h>
#include <instruction.h>

static void x86_emit_push_ebp(unsigned char *buffer)
{
	buffer[0] = 0x55;
}

static void x86_emit_mov_esp_ebp(unsigned char *buffer)
{
	buffer[0] = 0x89;
	buffer[1] = 0xe5;
}

int x86_emit_prolog(unsigned char *buffer, unsigned long buffer_size)
{
	if (buffer_size < 3)
		return -EINVAL;
	
	x86_emit_push_ebp(buffer);
	x86_emit_mov_esp_ebp(buffer + 1);
	
	return 0;
}

static void x86_emit_pop_ebp(unsigned char *buffer)
{
	buffer[0] = 0x5d;
}

static void x86_emit_ret(unsigned char *buffer)
{
	buffer[0] = 0xc3;
}

int x86_emit_epilog(unsigned char *buffer, unsigned long buffer_size)
{
	if (buffer_size < 2)
		return -EINVAL;

	x86_emit_pop_ebp(buffer);
	x86_emit_ret(buffer + 1);
	return 0;
}

static unsigned char register_to_modrm(enum reg reg)
{
	unsigned char ret = 0;

	switch (reg) {
	case REG_EAX:
		ret = 0x45;
		break;
	case REG_EBX:
		ret = 0x5d;
		break;
	case REG_ECX:
		ret = 0x4d;
		break;
	case REG_EDX:
		ret = 0x55;
		break;
	case REG_EBP:
		ret = 0x6d;
		break;
	}
	return ret;
}

static unsigned char to_x86_opcode(enum insn_opcode opcode)
{
	unsigned char ret = 0;

	switch (opcode) {
	case MOV:
		ret = 0x8b;
		break;
	case ADD:
		ret = 0x03;
		break;
	}
	return ret;
}

void x86_emit_obj_code(struct basic_block *bb, unsigned char *buffer,
		       unsigned long buffer_size)
{
	unsigned long offset = 0;
	struct insn *insn;

	list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
		buffer[offset] = to_x86_opcode(insn->insn_op);
		buffer[offset+1] = register_to_modrm(insn->dest.reg);
		buffer[offset+2] = insn->src.disp;
		offset += 3;
	}
}
