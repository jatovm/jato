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

/*
 *	encode_reg:	Encode register to be used in IA-32 instruction.
 *	@reg: Register to encode.
 *
 *	Returns register in r/m or reg/opcode field format of the ModR/M byte.
 */
static unsigned char encode_reg(enum reg reg)
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

/**
 *	x86_mod_rm:	Encode a ModR/M byte of an IA-32 instruction.
 *	@mod: The mod field of the byte.
 *	@reg_opcode: The reg/opcode field of the byte.
 *	@rm: The r/m field of the byte.
 */
static inline unsigned char x86_mod_rm(unsigned char mod,
				       unsigned char reg_opcode,
				       unsigned char rm)
{
	return ((mod & 0x3) << 6) | ((reg_opcode & 0x7) << 3) | (rm & 0x7);
}

static inline void x86_emit(struct insn_sequence *is, unsigned char c)
{
	assert(is->current != is->end);
	*(is->current++) = c;
}

static inline void x86_emit_disp8_reg(struct insn_sequence *is,
				      unsigned char opc, enum reg base_reg,
				      unsigned char disp8, enum reg dest_reg)
{
	unsigned char mod_rm;
	
	mod_rm = x86_mod_rm(0x01, encode_reg(dest_reg), encode_reg(base_reg));
	x86_emit(is, opc);
	x86_emit(is, mod_rm);
	x86_emit(is, disp8);
}

static void x86_emit_push_reg(struct insn_sequence *is, enum reg reg)
{
	x86_emit(is, 0x50 + encode_reg(reg));
}

static void x86_emit_mov_reg_reg(struct insn_sequence *is, enum reg src_reg,
				 enum reg dest_reg)
{
	unsigned char mod_rm;
	
	mod_rm = x86_mod_rm(0x03, encode_reg(src_reg), encode_reg(dest_reg));
	x86_emit(is, 0x89);
	x86_emit(is, mod_rm);
}

void x86_emit_mov_disp8_reg(struct insn_sequence *is, enum reg base_reg,
			    unsigned char disp8, enum reg dest_reg)
{
	x86_emit_disp8_reg(is, 0x8b, base_reg, disp8, dest_reg);
}

void x86_emit_prolog(struct insn_sequence *is)
{
	x86_emit_push_reg(is, REG_EBP);
	x86_emit_mov_reg_reg(is, REG_ESP, REG_EBP);
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

void x86_emit_push_imm32(struct insn_sequence *is, unsigned long imm)
{
	x86_emit(is, 0x68);
	x86_emit_imm32(is, imm);
}

#define CALL_INSN_SIZE 5

void x86_emit_call(struct insn_sequence *is, void *call_target)
{
	int disp = call_target - (void *) is->current - CALL_INSN_SIZE;

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

void x86_emit_add_disp8_reg(struct insn_sequence *is, enum reg base_reg,
			    unsigned char disp8, enum reg dest_reg)
{
	x86_emit_disp8_reg(is, 0x03, base_reg, disp8, dest_reg);
}

void x86_emit_add_imm8_reg(struct insn_sequence *is, unsigned char imm8,
			   enum reg reg)
{
	x86_emit(is, 0x83);
	x86_emit(is, x86_mod_rm(0x3, 0x00, encode_reg(reg)));
	x86_emit(is, imm8);
}

void x86_emit_cmp_disp8_reg(struct insn_sequence *is, enum reg base_reg,
			    unsigned char disp8, enum reg dest_reg)
{
	x86_emit_disp8_reg(is, 0x3b, base_reg, disp8, dest_reg);
}

void x86_emit_indirect_jump_reg(struct insn_sequence *is, enum reg reg)
{
	x86_emit(is, 0xff);
	x86_emit(is, x86_mod_rm(0x3, 0x04, encode_reg(reg)));
}

void x86_emit_je_rel(struct insn_sequence *is, unsigned char rel8)
{
	x86_emit(is, 0x74);
	x86_emit(is, rel8);
}

static void x86_emit_insn(struct insn_sequence *is, struct insn *insn)
{
	switch (insn->type) {
	case INSN_ADD_DISP_REG:
		x86_emit_add_disp8_reg(is, insn->src.reg, insn->src.disp,
				       insn->dest.reg);
		break;
	case INSN_ADD_IMM_REG:
		x86_emit_add_imm8_reg(is, insn->src.imm, insn->dest.reg);
		break;
	case INSN_CALL_REL:
		x86_emit_call(is, (void *)insn->operand.rel);
		break;
	case INSN_CMP_DISP_REG:
		x86_emit_cmp_disp8_reg(is, insn->src.reg, insn->src.disp,
				       insn->dest.reg);
		break;
	case INSN_JE_REL:
		x86_emit_je_rel(is, insn->operand.rel);
		break;
	case INSN_MOV_DISP_REG:
		x86_emit_mov_disp8_reg(is, insn->src.reg, insn->src.disp,
				       insn->dest.reg);
		break;
	case INSN_PUSH_IMM:
		x86_emit_push_imm32(is, insn->operand.imm);
		break;
	case INSN_PUSH_REG:
		x86_emit_push_reg(is, insn->operand.reg);
		break;
	default:
		assert(!"unknown opcode");
		break;
	}
}

void x86_emit_obj_code(struct basic_block *bb, struct insn_sequence *is)
{
	struct insn *insn;

	list_for_each_entry(insn, &bb->insn_list, insn_list_node)
		x86_emit_insn(is, insn);
}
