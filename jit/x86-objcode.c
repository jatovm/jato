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
#include <stdio.h>
#include <x86-objcode.h>
#include <jit/basic-block.h>
#include <jit/instruction.h>
#include <jit/statement.h>
#include <vm/buffer.h>

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

static inline int needs_32(long disp)
{
	return disp >> 8;
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

static inline unsigned char x86_sib(unsigned char scale, unsigned char index,
				    unsigned char base)
{
	return ((scale & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7);
}

static inline void x86_emit(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);
	assert(!err);
}

static void x86_emit_imm32(struct buffer *buf, int imm)
{
	union {
		int val;
		unsigned char b[4];
	} imm_buf;

	imm_buf.val = imm;
	x86_emit(buf, imm_buf.b[0]);
	x86_emit(buf, imm_buf.b[1]);
	x86_emit(buf, imm_buf.b[2]);
	x86_emit(buf, imm_buf.b[3]);
}

static void x86_emit_imm(struct buffer *buf, long disp)
{
	if (needs_32(disp))
		x86_emit_imm32(buf, disp);
	else
		x86_emit(buf, disp);
}

static void x86_emit_membase_reg(struct buffer *buf, unsigned char opc,
				 struct operand *src, struct operand *dest)
{
	enum reg base_reg, dest_reg;
	unsigned long disp;
	unsigned char mod, rm, mod_rm;
	int needs_sib, needs_32;

	base_reg = src->reg;
	disp = src->disp;
	dest_reg = dest->reg;
	
	needs_sib = (base_reg == REG_ESP);
	needs_32  = disp > 0xff;

	x86_emit(buf, opc);

	if (needs_sib)
		rm = 0x04;
	else
		rm = encode_reg(base_reg);

	if (needs_32)
		mod = 0x02;
	else
		mod = 0x01;

	mod_rm = x86_mod_rm(mod, encode_reg(dest_reg), rm);
	x86_emit(buf, mod_rm);

	if (needs_sib)
		x86_emit(buf, x86_sib(0x00, 0x04, encode_reg(base_reg)));

	x86_emit_imm(buf, disp);
}

static void x86_emit_push_reg(struct buffer *buf, enum reg reg)
{
	x86_emit(buf, 0x50 + encode_reg(reg));
}

static void x86_emit_mov_reg_reg(struct buffer *buf, enum reg src_reg,
				 enum reg dest_reg)
{
	unsigned char mod_rm;
	
	mod_rm = x86_mod_rm(0x03, encode_reg(src_reg), encode_reg(dest_reg));
	x86_emit(buf, 0x89);
	x86_emit(buf, mod_rm);
}

static void x86_emit_mov_membase_reg(struct buffer *buf,
				     struct operand *src, struct operand *dest)
{
	x86_emit_membase_reg(buf, 0x8b, src, dest);
}

void x86_emit_mov_imm32_reg(struct buffer *buf, unsigned long imm, enum reg reg)
{
	x86_emit(buf, 0xb8 + encode_reg(reg));
	x86_emit_imm32(buf, imm);
}

static void x86_emit_mov_imm_membase(struct buffer *buf,
				     struct operand *imm_operand,
				     struct operand *membase_operand)
{
	unsigned long mod = 0x00;

	x86_emit(buf, 0xc7);

	if (membase_operand->disp != 0)
		mod = 0x01;
	
	x86_emit(buf, x86_mod_rm(mod, 0x00, encode_reg(membase_operand->base_reg)));

	if (membase_operand->disp != 0)
		x86_emit(buf, membase_operand->disp);

	x86_emit_imm32(buf, imm_operand->imm);
}

static void x86_emit_mov_reg_membase(struct buffer *buf,
				     struct operand *reg_operand,
				     struct operand *membase_operand)
{
	int mod;

	if (needs_32(membase_operand->disp))
		mod = 0x02;
	else
		mod = 0x01;

	x86_emit(buf, 0x89);
	x86_emit(buf, x86_mod_rm(mod, encode_reg(reg_operand->reg), encode_reg(membase_operand->base_reg)));

	x86_emit_imm(buf, membase_operand->disp);
}

static void x86_emit_sub_imm_reg(struct buffer *buf, unsigned long imm, enum reg reg)
{
	x86_emit(buf, 0x81);
	x86_emit(buf, x86_mod_rm(0x03, 0x05, encode_reg(reg)));
	x86_emit_imm32(buf, imm);
}

void x86_emit_prolog(struct buffer *buf, unsigned long nr_locals)
{
	x86_emit_push_reg(buf, REG_EBP);
	x86_emit_mov_reg_reg(buf, REG_ESP, REG_EBP);

	if (nr_locals)
		x86_emit_sub_imm_reg(buf, nr_locals, REG_ESP);
}

static void x86_emit_pop_reg(struct buffer *buf, enum reg reg)
{
	x86_emit(buf, 0x58 + encode_reg(reg));
}

static void x86_emit_push_imm32(struct buffer *buf, unsigned long imm)
{
	x86_emit(buf, 0x68);
	x86_emit_imm32(buf, imm);
}

#define CALL_INSN_SIZE 5

static void x86_emit_call(struct buffer *buf, void *call_target)
{
	int disp = call_target - buffer_current(buf) - CALL_INSN_SIZE;

	x86_emit(buf, 0xe8);
	x86_emit_imm32(buf, disp);
}

void x86_emit_ret(struct buffer *buf)
{
	x86_emit(buf, 0xc3);
}

void x86_emit_epilog(struct buffer *buf, unsigned long nr_locals)
{
	if (nr_locals)
		x86_emit(buf, 0xc9);
	else
		x86_emit_pop_reg(buf, REG_EBP);

	x86_emit_ret(buf);
}

static void x86_emit_add_membase_reg(struct buffer *buf,
				     struct operand *src, struct operand *dest)
{
	x86_emit_membase_reg(buf, 0x03, src, dest);
}

static void x86_emit_sub_membase_reg(struct buffer *buf,
				     struct operand *src, struct operand *dest)
{
	x86_emit_membase_reg(buf, 0x2b, src, dest);
}

static void __x86_emit_div_mul_membase_reg(struct buffer *buf,
					   struct operand *src,
					   struct operand *dest,
					   unsigned char opc_ext)
{
	enum reg reg;
	long disp;
	int mod;

	assert(dest->reg == REG_EAX);

	reg  = src->reg;
	disp = src->disp;

	if (needs_32(disp))
		mod = 0x02;
	else
		mod = 0x01;

	x86_emit(buf, 0xf7);
	x86_emit(buf, x86_mod_rm(mod, opc_ext, encode_reg(reg)));
	x86_emit_imm(buf, disp);
}

static void x86_emit_mul_membase_reg(struct buffer *buf,
				     struct operand *src,
				     struct operand *dest)
{
	__x86_emit_div_mul_membase_reg(buf, src, dest, 0x04);
}

static void x86_emit_neg_reg(struct buffer *buf, struct operand *operand)
{
	x86_emit(buf, 0xf7);
	x86_emit(buf, x86_mod_rm(0x3, 0x3, encode_reg(operand->reg)));
}

static void x86_emit_cltd(struct buffer *buf)
{
	x86_emit(buf, 0x99);
}

static void x86_emit_div_membase_reg(struct buffer *buf, struct operand *src,
				     struct operand *dest)
{
	__x86_emit_div_mul_membase_reg(buf, src, dest, 0x07);
}

static void __x86_emit_shift_reg_reg(struct buffer *buf,
				     struct operand *src,
				     struct operand *dest,
				     unsigned char opc_ext)
{
	assert(src->reg == REG_ECX);

	x86_emit(buf, 0xd3);
	x86_emit(buf, x86_mod_rm(0x03, opc_ext, encode_reg(dest->reg)));
}

static void x86_emit_shl_reg_reg(struct buffer *buf, struct operand *src,
				 struct operand *dest)
{
	__x86_emit_shift_reg_reg(buf, src, dest, 0x04);
}

static void x86_emit_shr_reg_reg(struct buffer *buf, struct operand *src,
				 struct operand *dest)
{
	__x86_emit_shift_reg_reg(buf, src, dest, 0x05);
}

static void __x86_emit_add_imm_reg(struct buffer *buf, long imm, enum reg reg)
{
	int opc;

	if (needs_32(imm))
		opc = 0x81;
	else
		opc = 0x83;

	x86_emit(buf, opc);
	x86_emit(buf, x86_mod_rm(0x3, 0x00, encode_reg(reg)));
	x86_emit_imm(buf, imm);
}

static void x86_emit_add_imm_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	__x86_emit_add_imm_reg(buf, src->imm, dest->reg);
}

void x86_emit_cmp_imm_reg(struct buffer *buf,
			  struct operand *src, struct operand *dest)
{
	int opc;

	if (needs_32(src->imm))
		opc = 0x81;
	else 
		opc = 0x83;

	x86_emit(buf, opc);
	x86_emit(buf, x86_mod_rm(0x03, 0x07, encode_reg(dest->reg)));
	x86_emit_imm(buf, src->imm);
}

void x86_emit_cmp_membase_reg(struct buffer *buf, struct operand *src,
			      struct operand *dest)
{
	x86_emit_membase_reg(buf, 0x3b, src, dest);
}

static void x86_emit_indirect_jump_reg(struct buffer *buf, enum reg reg)
{
	x86_emit(buf, 0xff);
	x86_emit(buf, x86_mod_rm(0x3, 0x04, encode_reg(reg)));
}

void x86_emit_branch_rel(struct buffer *buf, unsigned char opc, unsigned char rel8)
{
	x86_emit(buf, opc);
	x86_emit(buf, rel8);
}

static unsigned char branch_rel_addr(unsigned long branch_offset,
				     unsigned long target_offset)
{
	return target_offset - branch_offset - 2;
}

static void x86_emit_branch(struct buffer *buf, unsigned char opc, struct insn *insn)
{
	struct basic_block *target_bb;
	unsigned char addr = 0;

	target_bb = insn->operand.branch_target;

	if (target_bb->is_emitted) {
		struct insn *target_insn =
			list_entry(target_bb->insn_list.next, struct insn,
				   insn_list_node);

		addr = branch_rel_addr(insn->offset, target_insn->offset);
	} else
		list_add(&insn->branch_list_node, &target_bb->backpatch_insns);

	x86_emit_branch_rel(buf, opc, addr);
}

static void x86_emit_indirect_call(struct buffer *buf,
				   struct operand *operand)
{
	x86_emit(buf, 0xff);
	x86_emit(buf, x86_mod_rm(0x0, 0x2, encode_reg(operand->reg)));
}

static void x86_emit_insn(struct buffer *buf, struct insn *insn)
{
	insn->offset = buffer_offset(buf); 

	switch (insn->type) {
	case INSN_ADD_MEMBASE_REG:
		x86_emit_add_membase_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_ADD_IMM_REG:
		x86_emit_add_imm_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_CALL_REG:
		x86_emit_indirect_call(buf, &insn->operand);
		break;
	case INSN_CALL_REL:
		x86_emit_call(buf, (void *)insn->operand.rel);
		break;
	case INSN_CLTD:
		x86_emit_cltd(buf);
		break;
	case INSN_CMP_IMM_REG:
		x86_emit_cmp_imm_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_CMP_MEMBASE_REG:
		x86_emit_cmp_membase_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_DIV_MEMBASE_REG:
		x86_emit_div_membase_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_JE_BRANCH:
		x86_emit_branch(buf, 0x74, insn);
		break;
	case INSN_JMP_BRANCH:
		x86_emit_branch(buf, 0xeb, insn);
		break;
	case INSN_MOV_MEMBASE_REG:
		x86_emit_mov_membase_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_MOV_IMM_REG:
		x86_emit_mov_imm32_reg(buf, insn->src.imm, insn->dest.reg);
		break;
	case INSN_MOV_IMM_MEMBASE:
		x86_emit_mov_imm_membase(buf, &insn->src, &insn->dest); 
		break;
	case INSN_MOV_REG_MEMBASE:
		x86_emit_mov_reg_membase(buf, &insn->src, &insn->dest);
		break;
	case INSN_MOV_REG_REG:
		x86_emit_mov_reg_reg(buf, insn->src.reg, insn->dest.reg);
		break;
	case INSN_MUL_MEMBASE_REG:
		x86_emit_mul_membase_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_NEG_REG:
		x86_emit_neg_reg(buf, &insn->operand);
		break;
	case INSN_PUSH_IMM:
		x86_emit_push_imm32(buf, insn->operand.imm);
		break;
	case INSN_PUSH_REG:
		x86_emit_push_reg(buf, insn->operand.reg);
		break;
	case INSN_SUB_MEMBASE_REG:
		x86_emit_sub_membase_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_SHL_REG_REG:
		x86_emit_shl_reg_reg(buf, &insn->src, &insn->dest);
		break;
	case INSN_SHR_REG_REG:
		x86_emit_shr_reg_reg(buf, &insn->src, &insn->dest);
		break;
	default:
		assert(!"unknown opcode");
		break;
	}
}

static void x86_backpatch_branches(struct buffer *buf,
				   struct list_head *to_backpatch,
				   unsigned long target_off)
{
	struct insn *this, *next;

	list_for_each_entry_safe(this, next, to_backpatch, branch_list_node) {
		unsigned long addr;

		addr = branch_rel_addr(this->offset, target_off);
		buf->buf[this->offset + 1] = addr;

		list_del(&this->branch_list_node);
	}
}

void x86_emit_obj_code(struct basic_block *bb, struct buffer *buf)
{
	struct insn *insn;

	x86_backpatch_branches(buf, &bb->backpatch_insns, buffer_offset(buf));

	list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
		x86_emit_insn(buf, insn);
	}
	bb->is_emitted = true;
}

void x86_emit_trampoline(struct compilation_unit *cu, void *call_target,
			 struct buffer *buf)
{
	x86_emit_push_imm32(buf, (unsigned long) cu);
	x86_emit_call(buf, call_target);
	__x86_emit_add_imm_reg(buf, 0x04, REG_ESP);
	x86_emit_indirect_jump_reg(buf, REG_EAX);
}

