/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for emitting IA-32 object code from IR
 * instruction sequence.
 */

#include <assert.h>
#include <errno.h>
#include <x86-objcode.h>
#include <basic-block.h>
#include <instruction.h>

static inline void x86_emit(struct insn_sequence *is, unsigned char c)
{
	assert(is->current != is->end);
	*(is->current++) = c;
}

static void x86_emit_push_ebp(struct insn_sequence *is)
{
	x86_emit(is, 0x55);
}

static void x86_emit_mov_esp_ebp(struct insn_sequence *is)
{
	x86_emit(is, 0x89);
	x86_emit(is, 0xe5);
}

void x86_emit_prolog(struct insn_sequence *is)
{
	x86_emit_push_ebp(is);
	x86_emit_mov_esp_ebp(is);
}

static void x86_emit_pop_ebp(struct insn_sequence *is)
{
	x86_emit(is, 0x5d);
}

static void x86_emit_ret(struct insn_sequence *is)
{
	x86_emit(is, 0xc3);
}

void x86_emit_epilog(struct insn_sequence *is)
{
	x86_emit_pop_ebp(is);
	x86_emit_ret(is);
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

void x86_emit_obj_code(struct basic_block *bb, struct insn_sequence *is)
{
	struct insn *insn;

	list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
		x86_emit(is, to_x86_opcode(insn->insn_op));
		x86_emit(is, register_to_modrm(insn->dest.reg));
		x86_emit(is, insn->src.disp);
	}
}
