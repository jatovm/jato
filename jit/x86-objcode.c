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

static void x86_emit_imm32(struct insn_sequence *is, int imm)
{
	union {
		int val;
		unsigned char b[4];
	} imm_buf;

	imm_buf.val = imm;
	x86_emit(is, imm_buf.b[0]);
	x86_emit(is, imm_buf.b[1]);
	x86_emit(is, imm_buf.b[2]);
	x86_emit(is, imm_buf.b[3]);
}

void x86_emit_push_imm32(struct insn_sequence *is, int imm)
{
	x86_emit(is, 0x68);
	x86_emit_imm32(is, imm);
}

#define INSN_CALL_SIZE 5

void x86_emit_call(struct insn_sequence *is, void *call_target)
{
	int disp = call_target - (void *) is->current - INSN_CALL_SIZE;

	x86_emit(is, 0xe8);
	x86_emit_imm32(is, disp);
}

void x86_emit_ret(struct insn_sequence *is)
{
	x86_emit(is, 0xc3);
}

void x86_emit_epilog(struct insn_sequence *is)
{
	x86_emit_pop_ebp(is);
	x86_emit_ret(is);
}

static unsigned char opcode_reg_mem(enum reg reg)
{
	unsigned char ret = 0;

	switch (reg) {
	case REG_EAX:
		ret = 0x00;
		break;
	case REG_EBX:
		ret = 0x03;
		break;
	case REG_ECX:
		ret = 0x01;
		break;
	case REG_EDX:
		ret = 0x02;
		break;
	case REG_ESP:
		ret = 0x04;
		break;
	case REG_EBP:
		ret = 0x05;
		break;
	}
	return ret;
}

static inline unsigned char x86_modrm(unsigned char mod,
				      unsigned char reg_opcode,
				      unsigned char reg_mem)
{
	return ((mod & 0x3) << 6) | ((reg_opcode & 0x7) << 3) | (reg_mem & 0x7);
}

void x86_emit_add_imm8_reg(struct insn_sequence *is, unsigned char imm8,
			   enum reg reg)
{
	x86_emit(is, 0x83);
	x86_emit(is, x86_modrm(0x3, 0x00, opcode_reg_mem(reg)));
	x86_emit(is, imm8);
}

void x86_emit_indirect_jump_reg(struct insn_sequence *is, enum reg reg)
{
	x86_emit(is, 0xff);
	x86_emit(is, x86_modrm(0x3, 0x04, opcode_reg_mem(reg)));
}

static unsigned char membase_ebp_imm8(enum reg reg)
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
	case REG_ESP:
		ret = 0x65;
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
	case INSN_CALL:
		assert(!"unknown opcode INSN_CALL");
		break;
	case INSN_PUSH:
		assert(!"unknwon opcode INSN_PUSH");
		break;
	}
	return ret;
}

void x86_emit_obj_code(struct basic_block *bb, struct insn_sequence *is)
{
	struct insn *insn;

	list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
		x86_emit(is, to_x86_opcode(insn->insn_op));
		x86_emit(is, membase_ebp_imm8(insn->dest.reg));
		x86_emit(is, insn->src.disp);
	}
}
