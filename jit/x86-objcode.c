/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for emitting IA-32 object code from IR
 * instruction sequence.
 */

#include <x86-objcode.h>
#include <basic-block.h>
#include <instruction.h>

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
	struct insn *insn = insn_entry(bb->insn_list.next);

	buffer[0] = to_x86_opcode(insn->insn_op);
	buffer[1] = register_to_modrm(insn->dest.reg);
	buffer[2] = insn->src.disp;
}
