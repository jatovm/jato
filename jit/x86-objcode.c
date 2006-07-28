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
#include <jit/basic-block.h>
#include <jit/instruction.h>
#include <jit/statement.h>

static inline unsigned long is_offset(struct insn_sequence *is)
{
	return is->current - is->start;
}

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

static inline void x86_emit(struct insn_sequence *is, unsigned char c)
{
	assert(is->current != is->end);
	*(is->current++) = c;
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

static void x86_emit_imm(struct insn_sequence *is, long disp)
{
	if (needs_32(disp))
		x86_emit_imm32(is, disp);
	else
		x86_emit(is, disp);
}

static void x86_emit_membase_reg(struct insn_sequence *is, unsigned char opc,
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

	x86_emit(is, opc);

	if (needs_sib)
		rm = 0x04;
	else
		rm = encode_reg(base_reg);

	if (needs_32)
		mod = 0x02;
	else
		mod = 0x01;

	mod_rm = x86_mod_rm(mod, encode_reg(dest_reg), rm);
	x86_emit(is, mod_rm);

	if (needs_sib)
		x86_emit(is, x86_sib(0x00, 0x04, encode_reg(base_reg)));

	x86_emit_imm(is, disp);
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

static void x86_emit_mov_membase_reg(struct insn_sequence *is,
				     struct operand *src, struct operand *dest)
{
	x86_emit_membase_reg(is, 0x8b, src, dest);
}

void x86_emit_mov_imm32_reg(struct insn_sequence *is, unsigned long imm, enum reg reg)
{
	x86_emit(is, 0xb8 + encode_reg(reg));
	x86_emit_imm32(is, imm);
}

static void x86_emit_mov_imm_membase(struct insn_sequence *is,
				     struct operand *imm_operand,
				     struct operand *membase_operand)
{
	unsigned long mod = 0x00;

	x86_emit(is, 0xc7);

	if (membase_operand->disp != 0)
		mod = 0x01;
	
	x86_emit(is, x86_mod_rm(mod, 0x00, encode_reg(membase_operand->base_reg)));

	if (membase_operand->disp != 0)
		x86_emit(is, membase_operand->disp);

	x86_emit_imm32(is, imm_operand->imm);
}

static void x86_emit_mov_reg_membase(struct insn_sequence *is,
				     struct operand *reg_operand,
				     struct operand *membase_operand)
{
	int mod;

	if (needs_32(membase_operand->disp))
		mod = 0x02;
	else
		mod = 0x01;

	x86_emit(is, 0x89);
	x86_emit(is, x86_mod_rm(mod, encode_reg(reg_operand->reg), encode_reg(membase_operand->base_reg)));

	x86_emit_imm(is, membase_operand->disp);
}

static void x86_emit_sub_imm_reg(struct insn_sequence *is, unsigned long imm, enum reg reg)
{
	x86_emit(is, 0x81);
	x86_emit(is, x86_mod_rm(0x03, 0x05, encode_reg(reg)));
	x86_emit_imm32(is, imm);
}

void x86_emit_prolog(struct insn_sequence *is, unsigned long nr_locals)
{
	x86_emit_push_reg(is, REG_EBP);
	x86_emit_mov_reg_reg(is, REG_ESP, REG_EBP);

	if (nr_locals)
		x86_emit_sub_imm_reg(is, nr_locals, REG_ESP);
}

static void x86_emit_pop_reg(struct insn_sequence *is, enum reg reg)
{
	x86_emit(is, 0x58 + encode_reg(reg));
}

static void x86_emit_push_imm32(struct insn_sequence *is, unsigned long imm)
{
	x86_emit(is, 0x68);
	x86_emit_imm32(is, imm);
}

#define CALL_INSN_SIZE 5

static void x86_emit_call(struct insn_sequence *is, void *call_target)
{
	int disp = call_target - (void *) is->current - CALL_INSN_SIZE;

	x86_emit(is, 0xe8);
	x86_emit_imm32(is, disp);
}

void x86_emit_ret(struct insn_sequence *is)
{
	x86_emit(is, 0xc3);
}

void x86_emit_epilog(struct insn_sequence *is, unsigned long nr_locals)
{
	if (nr_locals)
		x86_emit(is, 0xc9);
	else
		x86_emit_pop_reg(is, REG_EBP);

	x86_emit_ret(is);
}

static void x86_emit_add_membase_reg(struct insn_sequence *is,
				     struct operand *src, struct operand *dest)
{
	x86_emit_membase_reg(is, 0x03, src, dest);
}

static void __x86_emit_add_imm_reg(struct insn_sequence *is, long imm, enum reg reg)
{
	int opc;

	if (needs_32(imm))
		opc = 0x81;
	else
		opc = 0x83;

	x86_emit(is, opc);
	x86_emit(is, x86_mod_rm(0x3, 0x00, encode_reg(reg)));
	x86_emit_imm(is, imm);
}

static void x86_emit_add_imm_reg(struct insn_sequence *is,
				 struct operand *src, struct operand *dest)
{
	__x86_emit_add_imm_reg(is, src->imm, dest->reg);
}

void x86_emit_cmp_imm_reg(struct insn_sequence *is,
			  struct operand *src, struct operand *dest)
{
	int opc;

	if (needs_32(src->imm))
		opc = 0x81;
	else 
		opc = 0x83;

	x86_emit(is, opc);
	x86_emit(is, x86_mod_rm(0x03, 0x07, encode_reg(dest->reg)));
	x86_emit_imm(is, src->imm);
}

void x86_emit_cmp_membase_reg(struct insn_sequence *is, struct operand *src,
			      struct operand *dest)
{
	x86_emit_membase_reg(is, 0x3b, src, dest);
}

static void x86_emit_indirect_jump_reg(struct insn_sequence *is, enum reg reg)
{
	x86_emit(is, 0xff);
	x86_emit(is, x86_mod_rm(0x3, 0x04, encode_reg(reg)));
}

void x86_emit_je_rel(struct insn_sequence *is, unsigned char rel8)
{
	x86_emit(is, 0x74);
	x86_emit(is, rel8);
}

static unsigned char branch_rel_addr(unsigned long branch_offset,
				     unsigned long target_offset)
{
	return target_offset - branch_offset - 2;
}

static void x86_emit_branch(struct insn_sequence *is, struct insn *insn)
{
	struct basic_block *target_bb;
	unsigned char addr = 0;

	target_bb = insn->operand.branch_target->bb;

	if (!list_is_empty(&target_bb->insn_list)) {
		struct insn *target_insn =
			list_entry(target_bb->insn_list.next, struct insn,
				   insn_list_node);

		addr = branch_rel_addr(insn->offset, target_insn->offset);
	} else
		list_add(&insn->branch_list_node, &insn->operand.branch_target->branch_list);

	x86_emit_je_rel(is, addr);
}

static void x86_emit_indirect_jmp(struct insn_sequence *is,
				  struct operand *operand)
{
	x86_emit(is, 0xff);
	x86_emit(is, x86_mod_rm(3, 4, encode_reg(operand->reg)));
}

static void x86_emit_insn(struct insn_sequence *is, struct insn *insn)
{
	insn->offset = is_offset(is); 

	switch (insn->type) {
	case INSN_ADD_MEMBASE_REG:
		x86_emit_add_membase_reg(is, &insn->src, &insn->dest);
		break;
	case INSN_ADD_IMM_REG:
		x86_emit_add_imm_reg(is, &insn->src, &insn->dest);
		break;
	case INSN_CALL_REL:
		x86_emit_call(is, (void *)insn->operand.rel);
		break;
	case INSN_CMP_IMM_REG:
		x86_emit_cmp_imm_reg(is, &insn->src, &insn->dest);
		break;
	case INSN_CMP_MEMBASE_REG:
		x86_emit_cmp_membase_reg(is, &insn->src, &insn->dest);
		break;
	case INSN_JE_BRANCH:
		x86_emit_branch(is, insn);
		break;
	case INSN_JMP_REGISTER:
		x86_emit_indirect_jmp(is, &insn->operand);
		break;
	case INSN_MOV_MEMBASE_REG:
		x86_emit_mov_membase_reg(is, &insn->src, &insn->dest);
		break;
	case INSN_MOV_IMM_REG:
		x86_emit_mov_imm32_reg(is, insn->src.imm, insn->dest.reg);
		break;
	case INSN_MOV_IMM_MEMBASE:
		x86_emit_mov_imm_membase(is, &insn->src, &insn->dest); 
		break;
	case INSN_MOV_REG_MEMBASE:
		x86_emit_mov_reg_membase(is, &insn->src, &insn->dest);
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

static void x86_backpatch_branches(struct insn_sequence *is,
				   struct list_head *unresolved,
				   unsigned long target_off)
{
	struct insn *this, *next;

	list_for_each_entry_safe(this, next, unresolved, branch_list_node) {
		unsigned long addr;

		addr = branch_rel_addr(this->offset, target_off);
		is->start[this->offset + 1] = addr;

		list_del(&this->branch_list_node);
	}
}

void x86_emit_obj_code(struct basic_block *bb, struct insn_sequence *is)
{
	struct insn *insn;
	struct list_head *unresolved;

	unresolved = &bb->label_stmt->branch_list;
	x86_backpatch_branches(is, unresolved, is_offset(is));

	list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
		x86_emit_insn(is, insn);
	}
}

void x86_emit_trampoline(struct compilation_unit *cu, void *call_target,
			 void *buffer, unsigned long size)
{
	struct insn_sequence is;
	init_insn_sequence(&is, buffer, size);

	x86_emit_push_imm32(&is, (unsigned long) cu);
	x86_emit_call(&is, call_target);
	__x86_emit_add_imm_reg(&is, 0x04, REG_ESP);
	x86_emit_indirect_jump_reg(&is, REG_EAX);
}

