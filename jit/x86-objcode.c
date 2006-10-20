/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for emitting IA-32 object code from IR
 * instruction sequence.
 */

#include <jit/basic-block.h>
#include <jit/instruction.h>
#include <jit/statement.h>
#include <vm/buffer.h>
#include <x86-objcode.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>

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

static inline bool is_imm_8(long imm)
{
	return (imm >= -128) && (imm <= 127);
}

/**
 *	encode_modrm:	Encode a ModR/M byte of an IA-32 instruction.
 *	@mod: The mod field of the byte.
 *	@reg_opcode: The reg/opcode field of the byte.
 *	@rm: The r/m field of the byte.
 */
static inline unsigned char encode_modrm(unsigned char mod,
					 unsigned char reg_opcode,
					 unsigned char rm)
{
	return ((mod & 0x3) << 6) | ((reg_opcode & 0x7) << 3) | (rm & 0x7);
}

static inline unsigned char encode_sib(unsigned char scale,
				       unsigned char index, unsigned char base)
{
	return ((scale & 0x3) << 6) | ((index & 0x7) << 3) | (base & 0x7);
}

static inline void emit(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);
	assert(!err);
}

static void emit_imm32(struct buffer *buf, int imm)
{
	union {
		int val;
		unsigned char b[4];
	} imm_buf;

	imm_buf.val = imm;
	emit(buf, imm_buf.b[0]);
	emit(buf, imm_buf.b[1]);
	emit(buf, imm_buf.b[2]);
	emit(buf, imm_buf.b[3]);
}

static void emit_imm(struct buffer *buf, long imm)
{
	if (is_imm_8(imm))
		emit(buf, imm);
	else
		emit_imm32(buf, imm);
}

static void emit_membase_reg(struct buffer *buf, unsigned char opc,
			     struct operand *src, struct operand *dest)
{
	enum reg base_reg, dest_reg;
	unsigned long disp;
	unsigned char mod, rm, mod_rm;
	int needs_sib;

	base_reg = src->reg;
	disp = src->disp;
	dest_reg = dest->reg;

	needs_sib = (base_reg == REG_ESP);

	emit(buf, opc);

	if (needs_sib)
		rm = 0x04;
	else
		rm = encode_reg(base_reg);

	if (is_imm_8(disp))
		mod = 0x01;
	else
		mod = 0x02;

	mod_rm = encode_modrm(mod, encode_reg(dest_reg), rm);
	emit(buf, mod_rm);

	if (needs_sib)
		emit(buf, encode_sib(0x00, 0x04, encode_reg(base_reg)));

	emit_imm(buf, disp);
}

static void __emit_push_reg(struct buffer *buf, enum reg reg)
{
	emit(buf, 0x50 + encode_reg(reg));
}

static void emit_push_reg(struct buffer *buf, struct operand *operand)
{
	__emit_push_reg(buf, operand->reg);
}

static void __emit_mov_reg_reg(struct buffer *buf, enum reg src_reg,
			       enum reg dest_reg)
{
	unsigned char mod_rm;

	mod_rm = encode_modrm(0x03, encode_reg(src_reg), encode_reg(dest_reg));
	emit(buf, 0x89);
	emit(buf, mod_rm);
}

static void emit_mov_reg_reg(struct buffer *buf, struct operand *src,
			     struct operand *dest)
{
	__emit_mov_reg_reg(buf, src->reg, dest->reg);
}

static void emit_mov_membase_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x8b, src, dest);
}

static void emit_mov_imm_reg(struct buffer *buf, struct operand *src,
			     struct operand *dest)
{
	emit(buf, 0xb8 + encode_reg(dest->reg));
	emit_imm32(buf, src->imm);
}

static void emit_mov_imm_membase(struct buffer *buf, struct operand *src,
				 struct operand *dest)
{
	unsigned long mod = 0x00;

	emit(buf, 0xc7);

	if (dest->disp != 0)
		mod = 0x01;

	emit(buf, encode_modrm(mod, 0x00, encode_reg(dest->base_reg)));

	if (dest->disp != 0)
		emit(buf, dest->disp);

	emit_imm32(buf, src->imm);
}

static void emit_mov_reg_membase(struct buffer *buf, struct operand *src,
				 struct operand *dest)
{
	int mod;

	if (is_imm_8(dest->disp))
		mod = 0x01;
	else
		mod = 0x02;

	emit(buf, 0x89);
	emit(buf, encode_modrm(mod, encode_reg(src->reg),
			       encode_reg(dest->base_reg)));

	emit_imm(buf, dest->disp);
}

static void emit_sub_imm_reg(struct buffer *buf, unsigned long imm,
			     enum reg reg)
{
	emit(buf, 0x81);
	emit(buf, encode_modrm(0x03, 0x05, encode_reg(reg)));
	emit_imm32(buf, imm);
}

void emit_prolog(struct buffer *buf, unsigned long nr_locals)
{
	__emit_push_reg(buf, REG_EBP);
	__emit_mov_reg_reg(buf, REG_ESP, REG_EBP);

	if (nr_locals)
		emit_sub_imm_reg(buf, nr_locals, REG_ESP);
}

static void emit_pop_reg(struct buffer *buf, enum reg reg)
{
	emit(buf, 0x58 + encode_reg(reg));
}

static void __emit_push_imm(struct buffer *buf, long imm)
{
	emit(buf, 0x68);
	emit_imm(buf, imm);
}

static void emit_push_imm(struct buffer *buf, struct operand *operand)
{
	__emit_push_imm(buf, operand->imm);
}

#define CALL_INSN_SIZE 5

static void __emit_call(struct buffer *buf, void *call_target)
{
	int disp = call_target - buffer_current(buf) - CALL_INSN_SIZE;

	emit(buf, 0xe8);
	emit_imm32(buf, disp);
}

static void emit_call(struct buffer *buf, struct operand *operand)
{
	__emit_call(buf, (void *)operand->rel);
}

void emit_ret(struct buffer *buf)
{
	emit(buf, 0xc3);
}

void emit_epilog(struct buffer *buf, unsigned long nr_locals)
{
	if (nr_locals)
		emit(buf, 0xc9);
	else
		emit_pop_reg(buf, REG_EBP);

	emit_ret(buf);
}

static void emit_add_membase_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x03, src, dest);
}

static void emit_and_membase_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x23, src, dest);
}

static void emit_sub_membase_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x2b, src, dest);
}

static void __emit_div_mul_membase_reg(struct buffer *buf,
				       struct operand *src,
				       struct operand *dest,
				       unsigned char opc_ext)
{
	enum reg reg;
	long disp;
	int mod;

	assert(dest->reg == REG_EAX);

	reg = src->reg;
	disp = src->disp;

	if (is_imm_8(disp))
		mod = 0x01;
	else
		mod = 0x02;

	emit(buf, 0xf7);
	emit(buf, encode_modrm(mod, opc_ext, encode_reg(reg)));
	emit_imm(buf, disp);
}

static void emit_mul_membase_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	__emit_div_mul_membase_reg(buf, src, dest, 0x04);
}

static void emit_neg_reg(struct buffer *buf, struct operand *operand)
{
	emit(buf, 0xf7);
	emit(buf, encode_modrm(0x3, 0x3, encode_reg(operand->reg)));
}

static void emit_cltd(struct buffer *buf)
{
	emit(buf, 0x99);
}

static void emit_div_membase_reg(struct buffer *buf, struct operand *src,
				 struct operand *dest)
{
	__emit_div_mul_membase_reg(buf, src, dest, 0x07);
}

static void __emit_shift_reg_reg(struct buffer *buf,
				 struct operand *src,
				 struct operand *dest, unsigned char opc_ext)
{
	assert(src->reg == REG_ECX);

	emit(buf, 0xd3);
	emit(buf, encode_modrm(0x03, opc_ext, encode_reg(dest->reg)));
}

static void emit_shl_reg_reg(struct buffer *buf, struct operand *src,
			     struct operand *dest)
{
	__emit_shift_reg_reg(buf, src, dest, 0x04);
}

static void emit_sar_reg_reg(struct buffer *buf, struct operand *src,
			     struct operand *dest)
{
	__emit_shift_reg_reg(buf, src, dest, 0x07);
}

static void emit_shr_reg_reg(struct buffer *buf, struct operand *src,
			     struct operand *dest)
{
	__emit_shift_reg_reg(buf, src, dest, 0x05);
}

static void emit_or_membase_reg(struct buffer *buf,
				struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x0b, src, dest);
}

static void __emit_add_imm_reg(struct buffer *buf, long imm, enum reg reg)
{
	int opc;

	if (is_imm_8(imm))
		opc = 0x83;
	else
		opc = 0x81;

	emit(buf, opc);
	emit(buf, encode_modrm(0x3, 0x00, encode_reg(reg)));
	emit_imm(buf, imm);
}

static void emit_add_imm_reg(struct buffer *buf,
			     struct operand *src, struct operand *dest)
{
	__emit_add_imm_reg(buf, src->imm, dest->reg);
}

static void emit_cmp_imm_reg(struct buffer *buf, struct operand *src,
			     struct operand *dest)
{
	int opc;

	if (is_imm_8(src->imm))
		opc = 0x83;
	else
		opc = 0x81;

	emit(buf, opc);
	emit(buf, encode_modrm(0x03, 0x07, encode_reg(dest->reg)));
	emit_imm(buf, src->imm);
}

static void emit_cmp_membase_reg(struct buffer *buf, struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x3b, src, dest);
}

static void emit_indirect_jump_reg(struct buffer *buf, enum reg reg)
{
	emit(buf, 0xff);
	emit(buf, encode_modrm(0x3, 0x04, encode_reg(reg)));
}

void emit_branch_rel(struct buffer *buf, unsigned char opc, unsigned char rel8)
{
	emit(buf, opc);
	emit(buf, rel8);
}

static unsigned char branch_rel_addr(unsigned long branch_offset,
				     unsigned long target_offset)
{
	return target_offset - branch_offset - 2;
}

static void emit_branch(struct buffer *buf, unsigned char opc,
			struct insn *insn)
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

	emit_branch_rel(buf, opc, addr);
}

static void emit_indirect_call(struct buffer *buf, struct operand *operand)
{
	emit(buf, 0xff);
	emit(buf, encode_modrm(0x0, 0x2, encode_reg(operand->reg)));
}

static void emit_xor_membase_reg(struct buffer *buf,
				 struct operand *src, struct operand *dest)
{
	emit_membase_reg(buf, 0x33, src, dest);
}

struct emitter {
	void *emit_fn;
	int nr_operands;
};

#define DECL_EMITTER(_type, _fn, _nr) \
	[_type] = { .emit_fn = _fn, .nr_operands = _nr }

static struct emitter emitters[] = {
	DECL_EMITTER(INSN_ADD_IMM_REG, emit_add_imm_reg, 2),
	DECL_EMITTER(INSN_ADD_MEMBASE_REG, emit_add_membase_reg, 2),
	DECL_EMITTER(INSN_AND_MEMBASE_REG, emit_and_membase_reg, 2),
	DECL_EMITTER(INSN_CALL_REG, emit_indirect_call, 1),
	DECL_EMITTER(INSN_CALL_REL, emit_call, 1),
	DECL_EMITTER(INSN_CLTD, emit_cltd, 0),
	DECL_EMITTER(INSN_CMP_IMM_REG, emit_cmp_imm_reg, 2),
	DECL_EMITTER(INSN_CMP_MEMBASE_REG, emit_cmp_membase_reg, 2),
	DECL_EMITTER(INSN_DIV_MEMBASE_REG, emit_div_membase_reg, 2),
	DECL_EMITTER(INSN_MOV_IMM_MEMBASE, emit_mov_imm_membase, 2),
	DECL_EMITTER(INSN_MOV_IMM_REG, emit_mov_imm_reg, 2),
	DECL_EMITTER(INSN_MOV_MEMBASE_REG, emit_mov_membase_reg, 2),
	DECL_EMITTER(INSN_MOV_REG_MEMBASE, emit_mov_reg_membase, 2),
	DECL_EMITTER(INSN_MOV_REG_REG, emit_mov_reg_reg, 2),
	DECL_EMITTER(INSN_MUL_MEMBASE_REG, emit_mul_membase_reg, 2),
	DECL_EMITTER(INSN_NEG_REG, emit_neg_reg, 1),
	DECL_EMITTER(INSN_OR_MEMBASE_REG, emit_or_membase_reg, 2),
	DECL_EMITTER(INSN_PUSH_IMM, emit_push_imm, 1),
	DECL_EMITTER(INSN_PUSH_REG, emit_push_reg, 1),
	DECL_EMITTER(INSN_SAR_REG_REG, emit_sar_reg_reg, 2),
	DECL_EMITTER(INSN_SHL_REG_REG, emit_shl_reg_reg, 2),
	DECL_EMITTER(INSN_SHR_REG_REG, emit_shr_reg_reg, 2),
	DECL_EMITTER(INSN_SUB_MEMBASE_REG, emit_sub_membase_reg, 2),
	DECL_EMITTER(INSN_XOR_MEMBASE_REG, emit_xor_membase_reg, 2),
};

typedef void (*emit_fn_0) (struct buffer *);
typedef void (*emit_fn_1) (struct buffer *, struct operand * operand);
typedef void (*emit_fn_2) (struct buffer *, struct operand * src, struct operand
			   * dest);

static void emit_insn(struct buffer *buf, struct insn *insn)
{
	struct emitter *emitter;

	insn->offset = buffer_offset(buf);

	/*
	 * Branch emitters are special and need the insn.
	 */
	if (insn->type == INSN_JE_BRANCH) {
		emit_branch(buf, 0x74, insn);
		return;
	}
	if (insn->type == INSN_JMP_BRANCH) {
		emit_branch(buf, 0xeb, insn);
		return;
	}

	emitter = &emitters[insn->type];
	if (emitter->nr_operands == 0) {
		emit_fn_0 emit;
		emit = emitter->emit_fn;
		emit(buf);
	} else if (emitter->nr_operands == 1) {
		emit_fn_1 emit;
		emit = emitter->emit_fn;
		emit(buf, &insn->operand);
	} else {
		emit_fn_2 emit;
		emit = emitter->emit_fn;
		emit(buf, &insn->src, &insn->dest);
	}
}

static void backpatch_branches(struct buffer *buf,
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

void emit_obj_code(struct basic_block *bb, struct buffer *buf)
{
	struct insn *insn;

	backpatch_branches(buf, &bb->backpatch_insns, buffer_offset(buf));

	list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
		emit_insn(buf, insn);
	}
	bb->is_emitted = true;
}

void emit_trampoline(struct compilation_unit *cu, void *call_target,
		     struct buffer *buf)
{
	__emit_push_imm(buf, (unsigned long)cu);
	__emit_call(buf, call_target);
	__emit_add_imm_reg(buf, 0x04, REG_ESP);
	emit_indirect_jump_reg(buf, REG_EAX);
}
