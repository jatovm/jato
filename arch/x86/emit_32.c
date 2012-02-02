/*
 * x86-32/x86-64 code emitter.
 *
 * Copyright (C) 2006-2009 Pekka Enberg
 * Copyright (C) 2008-2009 Arthur Huillet
 * Copyright (C) 2009 Eduard - Gabriel Munteanu
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "arch/instruction.h"
#include "arch/stack-frame.h"
#include "arch/encode.h"
#include "arch/itable.h"
#include "arch/memory.h"
#include "arch/thread.h"
#include "arch/encode.h"
#include "arch/init.h"
#include "arch/inline-cache.h"

#include "cafebabe/method_info.h"

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/stack-slot.h"
#include "jit/statement.h"
#include "jit/compiler.h"
#include "jit/exception.h"
#include "jit/emit-code.h"
#include "jit/debug.h"
#include "jit/text.h"

#include "lib/buffer.h"
#include "lib/list.h"

#include "vm/backtrace.h"
#include "vm/method.h"
#include "vm/object.h"

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

/*
 * Common code emitters
 */

#define PREFIX_SIZE 1
#define BRANCH_INSN_SIZE 5
#define BRANCH_TARGET_OFFSET 1

#define PTR_SIZE	sizeof(long)

static unsigned char encode_reg(struct use_position *reg)
{
	return x86_encode_reg(mach_reg(reg));
}

static inline unsigned char reg_low(unsigned char reg)
{
	return reg & 0x7;
}

static inline unsigned char reg_high(unsigned char reg)
{
	return reg & 0x8;
}

static inline bool is_imm_8(long imm)
{
	return (imm >= -128) && (imm <= 127);
}

static inline void emit(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);
	assert(!err);
}

static void write_imm32(struct buffer *buf, unsigned long offset, long imm32)
{
	unsigned char *buffer;
	union {
		int val;
		unsigned char b[4];
	} imm_buf;

	buffer = buf->buf;
	imm_buf.val = imm32;

	buffer[offset] = imm_buf.b[0];
	buffer[offset + 1] = imm_buf.b[1];
	buffer[offset + 2] = imm_buf.b[2];
	buffer[offset + 3] = imm_buf.b[3];
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

static void __emit_call(struct buffer *buf, void *call_target)
{
	int disp = call_target - buffer_current(buf) - X86_CALL_INSN_SIZE;

	emit(buf, 0xe8);
	emit_imm32(buf, disp);
}

static void emit_call(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_call(buf, (void *)insn->operand.rel);
}

static void emit_ret(struct buffer *buf)
{
	emit(buf, 0xc3);
}

static void emit_leave(struct buffer *b)
{
	emit(b, 0xc9);
}

static void __emit_push_reg(struct buffer *buf, enum machine_reg reg)
{
	unsigned char rex_pfx = 0, rm;

	rm = x86_encode_reg(reg);

	if (rex_pfx)
		emit(buf, rex_pfx);

	emit(buf, 0x50 + reg_low(rm));
}

static void __emit_pop_reg(struct buffer *buf, enum machine_reg reg)
{
	unsigned char rex_pfx = 0, rm;

	rm = x86_encode_reg(reg);

	if (rex_pfx)
		emit(buf, rex_pfx);

	emit(buf, 0x58 + reg_low(rm));
}

static void __emit_push_imm(struct buffer *buf, long imm)
{
	unsigned char opc;

	if (is_imm_8(imm))
		opc = 0x6a;
	else
		opc = 0x68;

	emit(buf, opc);
	emit_imm(buf, imm);
}

static void emit_branch_rel(struct buffer *buf, unsigned char prefix,
			    unsigned char opc, long rel32)
{
	if (prefix)
		emit(buf, prefix);
	emit(buf, opc);
	emit_imm32(buf, rel32);
}

static long branch_rel_addr(struct insn *insn, unsigned long target_offset)
{
	long ret;

	ret = target_offset - insn->mach_offset - BRANCH_INSN_SIZE;
	if (insn->flags & INSN_FLAG_ESCAPED)
		ret -= PREFIX_SIZE;

	return ret;
}

static void __emit_branch(struct buffer *buf, struct basic_block *bb,
		unsigned char prefix, unsigned char opc, struct insn *insn)
{
	struct basic_block *target_bb;
	long addr = 0;
	int idx;

	if (prefix)
		insn->flags |= INSN_FLAG_ESCAPED;

	target_bb = insn->operand.branch_target;

	if (!bb)
		idx = -1;
	else
		idx = bb_lookup_successor_index(bb, target_bb);

	if (idx >= 0 && branch_needs_resolution_block(bb, idx)) {
		insn->flags |= INSN_FLAG_BACKPATCH_RESOLUTION;
		insn->operand.resolution_block = &bb->resolution_blocks[idx];
	} else if (target_bb->is_emitted) {
		struct insn *target_insn =
		    list_first_entry(&target_bb->insn_list, struct insn,
			       insn_list_node);

		addr = branch_rel_addr(insn, target_insn->mach_offset);
	} else
		insn->flags |= INSN_FLAG_BACKPATCH_BRANCH;

	emit_branch_rel(buf, prefix, opc, addr);
}

static void emit_je_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x0f, 0x84, insn);
}

static void emit_jne_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x0f, 0x85, insn);
}

static void emit_jge_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x0f, 0x8d, insn);
}

static void emit_jg_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x0f, 0x8f, insn);
}

static void emit_jle_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x0f, 0x8e, insn);
}

static void emit_jl_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x0f, 0x8c, insn);
}

static void emit_jmp_branch(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_branch(buf, bb, 0x00, 0xe9, insn);
}

void backpatch_branch_target(struct buffer *buf,
			     struct insn *insn,
			     unsigned long target_offset)
{
	unsigned long backpatch_offset;
	long relative_addr;

	backpatch_offset = insn->mach_offset + BRANCH_TARGET_OFFSET;
	if (insn->flags & INSN_FLAG_ESCAPED)
		backpatch_offset += PREFIX_SIZE;

	relative_addr = branch_rel_addr(insn, target_offset);

	write_imm32(buf, backpatch_offset, relative_addr);
}

static void __emit_jmp(struct buffer *buf, unsigned long addr)
{
	unsigned long current = (unsigned long)buffer_current(buf);
	emit(buf, 0xE9);
	emit_imm32(buf, addr - current - BRANCH_INSN_SIZE);
}

static void __emit_mov_imm_reg(struct buffer *buf, long imm, enum machine_reg reg)
{
	emit(buf, 0xb8 + x86_encode_reg(reg));
	emit_imm32(buf, imm);
}

static void fixup_branch_target(uint8_t *target_p, void *target)
{
	long cur = (long) (target - (void *) target_p) - 4;
	target_p[3] = cur >> 24;
	target_p[2] = cur >> 16;
	target_p[1] = cur >> 8;
	target_p[0] = cur;
}

static void emit_really_indirect_jump_reg(struct buffer *buf, enum machine_reg reg)
{
	emit(buf, 0xff);
	emit(buf, x86_encode_mod_rm(0x0, 0x04, x86_encode_reg(reg)));
}

static void emit_mov_reg_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb);
static void emit_mov_imm_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb);

static void __emit_mov_imm_membase(struct buffer *buf, long imm,
				   enum machine_reg base, long disp);

static void
__emit_reg_reg(struct buffer *buf, unsigned char opc,
	       enum machine_reg direct_reg, enum machine_reg rm_reg)
{
	unsigned char mod_rm;

	mod_rm = x86_encode_mod_rm(0x03, x86_encode_reg(direct_reg), x86_encode_reg(rm_reg));

	emit(buf, opc);
	emit(buf, mod_rm);
}

static void
__emit_cmp_reg_reg(struct buffer *buf,
		enum machine_reg reg1, enum machine_reg reg2)
{
	__emit_reg_reg(buf, 0x39, reg1, reg2);
}

static void
emit_reg_reg(struct buffer *buf, unsigned char opc,
	     struct operand *direct, struct operand *rm)
{
	enum machine_reg direct_reg, rm_reg;

	direct_reg = mach_reg(&direct->reg);
	rm_reg = mach_reg(&rm->reg);

	__emit_reg_reg(buf, opc, direct_reg, rm_reg);
}

static void
__emit_memdisp(struct buffer *buf, unsigned char opc, unsigned long disp,
	       unsigned char reg_opcode)
{
	unsigned char mod_rm;

	mod_rm = x86_encode_mod_rm(0, reg_opcode, 5);

	emit(buf, opc);
	emit(buf, mod_rm);
	emit_imm32(buf, disp);
}

static void
__emit_memdisp_reg(struct buffer *buf, unsigned char opc, unsigned long disp,
		   enum machine_reg reg)
{
	__emit_memdisp(buf, opc, disp, x86_encode_reg(reg));
}

static void
__emit_reg_memdisp(struct buffer *buf, unsigned char opc, enum machine_reg reg,
		   unsigned long disp)
{
	__emit_memdisp(buf, opc, disp, x86_encode_reg(reg));
}

static void
__emit_membase(struct buffer *buf, unsigned char opc,
	       enum machine_reg base_reg, unsigned long disp,
	       unsigned char reg_opcode)
{
	unsigned char mod, rm, mod_rm;
	int needs_sib;

	needs_sib = (base_reg == MACH_REG_ESP);

	emit(buf, opc);

	if (needs_sib)
		rm = 0x04;
	else
		rm = x86_encode_reg(base_reg);

	if (disp == 0 && base_reg != MACH_REG_EBP)
		mod = 0x00;
	else if (is_imm_8(disp))
		mod = 0x01;
	else
		mod = 0x02;

	mod_rm = x86_encode_mod_rm(mod, reg_opcode, rm);
	emit(buf, mod_rm);

	if (needs_sib)
		emit(buf, x86_encode_sib(0x00, 0x04, x86_encode_reg(base_reg)));

	if (disp)
		emit_imm(buf, disp);
}

static void
__emit_membase_reg(struct buffer *buf, unsigned char opc,
		   enum machine_reg base_reg, unsigned long disp,
		   enum machine_reg dest_reg)
{
	__emit_membase(buf, opc, base_reg, disp, x86_encode_reg(dest_reg));
}

static void __emit_push_membase(struct buffer *buf, enum machine_reg src_reg,
				unsigned long disp)
{
	__emit_membase(buf, 0xff, src_reg, disp, 6);
}

static void __emit_mov_reg_reg(struct buffer *buf, enum machine_reg src_reg,
			       enum machine_reg dest_reg)
{
	__emit_reg_reg(buf, 0x89, src_reg, dest_reg);
}

static void emit_mov_memlocal_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	enum machine_reg dest_reg;
	unsigned long disp;

	dest_reg = mach_reg(&insn->dest.reg);
	disp = slot_offset(insn->src.slot);

	__emit_membase_reg(buf, 0x8b, MACH_REG_EBP, disp, dest_reg);
}

static void emit_mov_thread_local_memdisp_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x65); /* GS segment override prefix */
	__emit_memdisp_reg(buf, 0x8b, insn->src.imm, mach_reg(&insn->dest.reg));
}

static void emit_mov_reg_thread_local_memdisp(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x65); /* GS segment override prefix */
	__emit_reg_memdisp(buf, 0x89, mach_reg(&insn->src.reg), insn->dest.imm);
}

static void emit_mov_reg_thread_local_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x65); /* GS segment override prefix */
	emit_mov_reg_membase(insn, buf, bb);
}

static void emit_mov_imm_thread_local_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x65); /* GS segment override prefix */
	emit_mov_imm_membase(insn, buf, bb);
}

static void emit_mov_memdisp_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_memdisp_reg(buf, 0x8b, insn->src.imm, mach_reg(&insn->dest.reg));
}

static void emit_mov_reg_memdisp(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_reg_memdisp(buf, 0x89, mach_reg(&insn->src.reg), insn->dest.imm);
}

static void emit_mov_memindex_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x8b);
	emit(buf, x86_encode_mod_rm(0x00, encode_reg(&insn->dest.reg), 0x04));
	emit(buf, x86_encode_sib(insn->src.shift, encode_reg(&insn->src.index_reg), encode_reg(&insn->src.base_reg)));
}

static void emit_mov_imm_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_mov_imm_reg(buf, insn->src.imm, mach_reg(&insn->dest.reg));
}

static void __emit_mov_imm_membase(struct buffer *buf, long imm,
				   enum machine_reg base, long disp)
{
	__emit_membase(buf, 0xc7, base, disp, 0);
	emit_imm32(buf, imm);
}

static void emit_mov_imm_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_mov_imm_membase(buf, insn->src.imm, mach_reg(&insn->dest.base_reg), insn->dest.disp);
}

static void emit_mov_imm_memlocal(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_mov_imm_membase(buf, insn->src.imm, MACH_REG_EBP, slot_offset(insn->dest.slot));
}

static void __emit_mov_reg_membase(struct buffer *buf, enum machine_reg src,
				   enum machine_reg base, unsigned long disp)
{
	__emit_membase(buf, 0x89, base, disp, x86_encode_reg(src));
}

static void emit_mov_reg_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_mov_reg_membase(buf, mach_reg(&insn->src.reg), mach_reg(&insn->dest.base_reg), insn->dest.disp);
}

static void emit_mov_reg_memlocal(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_mov_reg_membase(buf, mach_reg(&insn->src.reg), MACH_REG_EBP, slot_offset(insn->dest.slot));
}

static void emit_mov_reg_memindex(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x89);
	emit(buf, x86_encode_mod_rm(0x00, encode_reg(&insn->src.reg), 0x04));
	emit(buf, x86_encode_sib(insn->dest.shift, encode_reg(&insn->dest.index_reg), encode_reg(&insn->dest.base_reg)));
}

static void emit_alu_imm_reg(struct buffer *buf, unsigned char opc_ext,
			     long imm, enum machine_reg reg)
{
	int opc;

	if (is_imm_8(imm))
		opc = 0x83;
	else
		opc = 0x81;

	emit(buf, opc);
	emit(buf, x86_encode_mod_rm(0x3, opc_ext, x86_encode_reg(reg)));
	emit_imm(buf, imm);
}

static void __emit_sub_imm_reg(struct buffer *buf, unsigned long imm,
			       enum machine_reg reg)
{
	emit_alu_imm_reg(buf, 0x05, imm, reg);
}

static void __emit_cmp_imm_reg(struct buffer *buf, int rex_w, long imm, enum machine_reg reg)
{
	emit_alu_imm_reg(buf, 0x07, imm, reg);
}

static void __emit_test_imm_memdisp(struct buffer *buf,
	long imm, long disp)
{
	/* XXX: Supports only byte or long imms */

	if (is_imm_8(imm))
		emit(buf, 0xf6);
	else
		emit(buf, 0xf7);

	emit(buf, 0x04);
	emit(buf, 0x25);
	emit_imm32(buf, disp);
	emit_imm(buf, imm);
}

static void emit_test_imm_memdisp(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_test_imm_memdisp(buf, insn->src.imm, insn->dest.disp);
}

static void emit_save_callee_save_regs(struct buffer *buf)
{
	int i;

	for (i = 0; i < NR_CALLEE_SAVE_REGS; i++) {
		enum machine_reg reg = callee_save_regs[i];

		__emit_push_reg(buf, reg);
	}
}

static void emit_restore_callee_save_regs(struct buffer *buf)
{
	int i;

	for (i = 0; i < NR_CALLEE_SAVE_REGS; i++) {
		enum machine_reg reg = callee_save_regs[NR_CALLEE_SAVE_REGS - i - 1];

		__emit_pop_reg(buf, reg);
	}
}

#define STACK_FRAME_REDZONE_END		0xdeadbeef

void emit_prolog(struct buffer *buf, struct stack_frame *frame,
					unsigned long frame_size)
{
	emit_save_callee_save_regs(buf);

	__emit_push_reg(buf, MACH_REG_EBP);
	__emit_mov_reg_reg(buf, MACH_REG_ESP, MACH_REG_EBP);

	if (frame_size)
		__emit_sub_imm_reg(buf, frame_size, MACH_REG_ESP);

	if (opt_debug_stack)
		__emit_push_imm(buf, STACK_FRAME_REDZONE_END);
}

/* call-site in edx, magic is in ecx */
void __attribute__((regparm(3)))
stack_frame_redzone_fail(void *eax, void *edx, void *ecx)
{
	printf("Stack frame redzone overwritten at %p: %p\n", edx, ecx);
	abort();
}

static void emit_stack_redzone_check(struct buffer *buf)
{
	__emit_mov_imm_reg(buf, (unsigned long) buffer_current(buf), MACH_REG_EDX);
	__emit_pop_reg(buf, MACH_REG_ECX);

	__emit_cmp_imm_reg(buf, 1, STACK_FRAME_REDZONE_END, MACH_REG_ECX);

	/* open-coded "jne" */
	emit(buf, 0x0f);
	emit(buf, 0x85);

	uint8_t *jne_addr = buffer_current(buf);
	emit_imm32(buf, 0);
	fixup_branch_target(jne_addr, stack_frame_redzone_fail);
}

static void emit_restore_regs(struct buffer *buf)
{
	emit_restore_callee_save_regs(buf);
}

void emit_epilog(struct buffer *buf)
{
	if (opt_debug_stack)
		emit_stack_redzone_check(buf);

	emit_leave(buf);
	emit_restore_regs(buf);
	emit_ret(buf);
}

/*
 * FIXME: We use different stack layout on 32-bit and 64-bit so prolog, epilog,
 * and unwind sequences are slightly different.
 */
void emit_unwind(struct buffer *buf)
{
	if (opt_debug_stack)
		emit_stack_redzone_check(buf);

	emit_leave(buf);
	emit_restore_regs(buf);
	__emit_jmp(buf, (unsigned long)&unwind);
}

static void emit_push_imm(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_push_imm(buf, insn->operand.imm);
}

static void emit_fld_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xd9, mach_reg(&insn->operand.base_reg), insn->operand.disp, 0);
}

static void emit_fld_64_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xdd, mach_reg(&insn->operand.base_reg), insn->operand.disp, 0);
}

static void emit_fild_64_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xdf, mach_reg(&insn->operand.base_reg), insn->operand.disp, 5);
}

static void emit_fldcw_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xd9, mach_reg(&insn->operand.base_reg), insn->operand.disp, 5);
}

static void emit_fnstcw_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xd9, mach_reg(&insn->operand.base_reg), insn->operand.disp, 7);
}

static void emit_fistp_64_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xdf, mach_reg(&insn->operand.base_reg), insn->operand.disp, 7);
}

static void emit_fstp_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xd9, mach_reg(&insn->operand.base_reg), insn->operand.disp, 3);
}

static void emit_fstp_64_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0xdd, mach_reg(&insn->operand.base_reg), insn->operand.disp, 3);
}

static void __emit_div_mul_membase_eax(struct buffer *buf,
				       struct operand *src,
				       struct operand *dest,
				       unsigned char opc_ext)
{
	long disp;
	int mod;

	assert(mach_reg(&dest->reg) == MACH_REG_EAX);

	disp = src->disp;

	if (is_imm_8(disp))
		mod = 0x01;
	else
		mod = 0x02;

	emit(buf, 0xf7);
	emit(buf, x86_encode_mod_rm(mod, opc_ext, encode_reg(&src->base_reg)));
	emit_imm(buf, disp);
}

static void __emit_div_mul_reg_eax(struct buffer *buf,
				       struct operand *src,
				       struct operand *dest,
				       unsigned char opc_ext)
{
	assert(mach_reg(&dest->reg) == MACH_REG_EAX);

	emit(buf, 0xf7);
	emit(buf, x86_encode_mod_rm(0x03, opc_ext, encode_reg(&src->base_reg)));
}

static void emit_mul_membase_eax(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_div_mul_membase_eax(buf, &insn->src, &insn->dest, 0x04);
}

static void emit_mul_reg_eax(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_div_mul_reg_eax(buf, &insn->src, &insn->dest, 0x04);
}

static void emit_mul_reg_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0x0f);
	__emit_reg_reg(buf, 0xaf, mach_reg(&insn->dest.reg), mach_reg(&insn->src.reg));
}

static void emit_div_membase_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_div_mul_membase_eax(buf, &insn->src, &insn->dest, 0x07);
}

static void emit_div_reg_reg(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_div_mul_reg_eax(buf, &insn->src, &insn->dest, 0x07);
}

static void emit_or_imm_membase(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	__emit_membase(buf, 0x81, mach_reg(&insn->dest.base_reg), insn->dest.disp, 1);
	emit_imm32(buf, insn->src.disp);
}

static void __emit_add_imm_reg(struct buffer *buf, long imm, enum machine_reg reg)
{
	emit_alu_imm_reg(buf, 0x00, imm, reg);
}

static void emit_indirect_jump_reg(struct buffer *buf, enum machine_reg reg)
{
	emit(buf, 0xff);
	emit(buf, x86_encode_mod_rm(0x3, 0x04, x86_encode_reg(reg)));
}

static void __emit_test_membase_reg(struct buffer *buf, enum machine_reg src,
				    unsigned long disp, enum machine_reg dest)
{
	__emit_membase_reg(buf, 0x85, src, disp, dest);
}

/* Emits exception test using given register. */
static void emit_exception_test(struct buffer *buf, enum machine_reg reg)
{
	/* mov gs:(0xXXX), %reg */
	emit(buf, 0x65);
	__emit_memdisp_reg(buf, 0x8b,
		get_thread_local_offset(&exception_guard), reg);

	/* test (%reg), %reg */
	__emit_test_membase_reg(buf, reg, 0, reg);
}

static void emit_conv_xmm_to_xmm64(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf3);
	emit(buf, 0x0f);
	emit_reg_reg(buf, 0x5a, &insn->dest, &insn->src);
}

static void emit_conv_xmm64_to_xmm(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf2);
	emit(buf, 0x0f);
	emit_reg_reg(buf, 0x5a, &insn->dest, &insn->src);
}

static void emit_conv_gpr_to_fpu(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf3);
	emit(buf, 0x0f);
	emit_reg_reg(buf, 0x2a, &insn->dest, &insn->src);
}

static void emit_conv_gpr_to_fpu64(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf2);
	emit(buf, 0x0f);
	emit_reg_reg(buf, 0x2a, &insn->dest, &insn->src);
}

static void emit_conv_fpu_to_gpr(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf3);
	emit(buf, 0x0f);
	emit_reg_reg(buf, 0x2d, &insn->dest, &insn->src);
}

static void emit_conv_fpu64_to_gpr(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf2);
	emit(buf, 0x0f);
	emit_reg_reg(buf, 0x2d, &insn->dest, &insn->src);
}

static void emit_mov_memindex_xmm(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf3);
	emit(buf, 0x0f);
	emit(buf, 0x10);
	emit(buf, x86_encode_mod_rm(0x00, encode_reg(&insn->dest.reg), 0x04));
	emit(buf, x86_encode_sib(insn->src.shift, encode_reg(&insn->src.index_reg), encode_reg(&insn->src.base_reg)));
}

static void emit_mov_64_memindex_xmm(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf2);
	emit(buf, 0x0f);
	emit(buf, 0x10);
	emit(buf, x86_encode_mod_rm(0x00, encode_reg(&insn->dest.reg), 0x04));
	emit(buf, x86_encode_sib(insn->src.shift, encode_reg(&insn->src.index_reg), encode_reg(&insn->src.base_reg)));
}

static void emit_mov_xmm_memindex(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf3);
	emit(buf, 0x0f);
	emit(buf, 0x11);
	emit(buf, x86_encode_mod_rm(0x00, encode_reg(&insn->src.reg), 0x04));
	emit(buf, x86_encode_sib(insn->dest.shift, encode_reg(&insn->dest.index_reg), encode_reg(&insn->dest.base_reg)));
}

static void emit_mov_64_xmm_memindex(struct insn *insn, struct buffer *buf, struct basic_block *bb)
{
	emit(buf, 0xf2);
	emit(buf, 0x0f);
	emit(buf, 0x11);
	emit(buf, x86_encode_mod_rm(0x00, encode_reg(&insn->src.reg), 0x04));
	emit(buf, x86_encode_sib(insn->dest.shift, encode_reg(&insn->dest.index_reg), encode_reg(&insn->dest.base_reg)));
}

void emit_trace_invoke(struct buffer *buf, struct compilation_unit *cu)
{
	__emit_push_imm(buf, (unsigned long) cu);
	__emit_call(buf, &trace_invoke);
	__emit_add_imm_reg(buf, PTR_SIZE, MACH_REG_xSP);
}

void emit_trampoline(struct compilation_unit *cu,
		     void *call_target,
		     struct jit_trampoline *trampoline)
{
	struct buffer *buf = trampoline->objcode;

	jit_text_lock();

	buf->buf = jit_text_ptr();

	/* This is for __builtin_return_address() to work and to access
	   call arguments in correct manner. */
	__emit_push_reg(buf, MACH_REG_EBP);
	__emit_mov_reg_reg(buf, MACH_REG_ESP, MACH_REG_EBP);

	__emit_push_imm(buf, (unsigned long)cu);
	__emit_call(buf, call_target);
	__emit_add_imm_reg(buf, 0x04, MACH_REG_ESP);

	/*
	 * Test for exeption occurrence.
	 * We do this by polling a dedicated thread-specific pointer,
	 * which triggers SIGSEGV when exception is set.
	 *
	 * mov gs:(0xXXX), %ecx
	 * test (%ecx), %ecx
	 */
	emit(buf, 0x65);
	__emit_memdisp_reg(buf, 0x8b,
			   get_thread_local_offset(&trampoline_exception_guard),
			   MACH_REG_ECX);
	__emit_test_membase_reg(buf, MACH_REG_ECX, 0, MACH_REG_ECX);

	if (method_is_virtual(cu->method)) {
		__emit_push_reg(buf, MACH_REG_EAX);
		__emit_push_membase(buf, MACH_REG_EBP, 0x08);

		__emit_push_imm(buf, (unsigned long)cu);
		__emit_call(buf, fixup_vtable);
		__emit_add_imm_reg(buf, 0x08, MACH_REG_ESP);
		__emit_pop_reg(buf, MACH_REG_EAX);
	}

	__emit_pop_reg(buf, MACH_REG_EBP);
	emit_indirect_jump_reg(buf, MACH_REG_EAX);

	jit_text_reserve(buffer_offset(buf));
	jit_text_unlock();
}

void emit_lock(struct buffer *buf, struct vm_object *obj)
{
	__emit_push_imm(buf, (unsigned long)obj);
	__emit_call(buf, vm_object_lock);
	__emit_add_imm_reg(buf, PTR_SIZE, MACH_REG_xSP);

	__emit_push_reg(buf, MACH_REG_EAX);
	emit_exception_test(buf, MACH_REG_EAX);
	__emit_pop_reg(buf, MACH_REG_EAX);
}

void emit_unlock(struct buffer *buf, struct vm_object *obj)
{
	/* Save caller-saved registers which contain method's return value */
	__emit_push_reg(buf, MACH_REG_EAX);
	__emit_push_reg(buf, MACH_REG_EDX);

	__emit_push_imm(buf, (unsigned long)obj);
	__emit_call(buf, vm_object_unlock);
	__emit_add_imm_reg(buf, PTR_SIZE, MACH_REG_ESP);

	emit_exception_test(buf, MACH_REG_EAX);

	__emit_pop_reg(buf, MACH_REG_EDX);
	__emit_pop_reg(buf, MACH_REG_EAX);
}

void emit_lock_this(struct buffer *buf, unsigned long frame_size)
{
	unsigned long this_arg_offset;

	this_arg_offset = offsetof(struct jit_stack_frame, args);

	__emit_push_membase(buf, MACH_REG_EBP, this_arg_offset);
	__emit_call(buf, vm_object_lock);
	__emit_add_imm_reg(buf, PTR_SIZE, MACH_REG_ESP);

	__emit_push_reg(buf, MACH_REG_EAX);
	emit_exception_test(buf, MACH_REG_EAX);
	__emit_pop_reg(buf, MACH_REG_EAX);
}

void emit_unlock_this(struct buffer *buf, unsigned long frame_size)
{
	unsigned long this_arg_offset;

	this_arg_offset = offsetof(struct jit_stack_frame, args);

	/* Save caller-saved registers which contain method's return value */
	__emit_push_reg(buf, MACH_REG_EAX);
	__emit_push_reg(buf, MACH_REG_EDX);

	__emit_push_membase(buf, MACH_REG_EBP, this_arg_offset);
	__emit_call(buf, vm_object_unlock);
	__emit_add_imm_reg(buf, PTR_SIZE, MACH_REG_ESP);

	emit_exception_test(buf, MACH_REG_EAX);

	__emit_pop_reg(buf, MACH_REG_EDX);
	__emit_pop_reg(buf, MACH_REG_EAX);
}

void *emit_ic_check(struct buffer *buf)
{
	void *jne_addr;

	__emit_cmp_reg_reg(buf, MACH_REG_EAX, MACH_REG_ECX);

	/* open-coded "jne" */
	emit(buf, 0x0f);
	emit(buf, 0x85);

	jne_addr = buffer_current(buf);

	emit_imm32(buf, 0);

	return jne_addr;
}

void emit_ic_miss_handler(struct buffer *buf, void *ic_check, struct vm_method *vmm)
{
	fixup_branch_target(ic_check, buffer_current(buf));

	__emit_push_membase(buf, MACH_REG_ESP, 0);
	__emit_push_imm(buf, (long)vmm);
	__emit_push_reg(buf, MACH_REG_ECX);
	__emit_call(buf, (void *)resolve_ic_miss);
	__emit_add_imm_reg(buf, 12, MACH_REG_ESP);
	emit_indirect_jump_reg(buf, MACH_REG_EAX);
}

extern void jni_trampoline(void);

void emit_jni_trampoline(struct buffer *buf, struct vm_method *vmm,
			 void *target)
{
	jit_text_lock();

	buf->buf = jit_text_ptr();

	__emit_pop_reg(buf, MACH_REG_xAX);	/* return address */

	__emit_push_reg(buf, MACH_REG_xAX);
	__emit_push_imm(buf, (unsigned long) target);
	__emit_push_reg(buf, MACH_REG_xAX);
	__emit_push_imm(buf, (unsigned long) vmm);
	__emit_push_reg(buf, MACH_REG_xBP);
	__emit_jmp(buf, (unsigned long) jni_trampoline);

	jit_text_reserve(buffer_offset(buf));
	jit_text_unlock();
}

/* The regparm(1) makes GCC get the first argument from %ecx and the rest
 * from the stack. This is convenient, because we use %ecx for passing the
 * hidden "method" parameter. Interfaces are invoked on objects, so we also
 * always get the object in the first stack parameter. */
void __attribute__((regparm(1)))
itable_resolver_stub_error(struct vm_method *method, struct vm_object *obj)
{
	fprintf(stderr, "itable resolver stub error!\n");
	fprintf(stderr, "invokeinterface called on method %s.%s%s "
		"(itable index %d)\n",
		method->class->name, method->name, method->type,
		method->itable_index);
	fprintf(stderr, "object class %s\n", obj->class->name);

	print_trace();
	abort();
}

/* Note: a < b, always */
static void emit_itable_bsearch(struct buffer *buf,
	struct itable_entry **table, unsigned int a, unsigned int b)
{
	uint8_t *jb_addr = NULL;
	uint8_t *ja_addr = NULL;
	unsigned int m;

	/* Find middle (safe from overflows) */
	m = a + (b - a) / 2;

	/* No point in emitting the "cmp" if we're not going to test
	 * anything */
	if (b - a >= 1) {
		__emit_cmp_imm_reg(buf, 1, (long) table[m]->i_method, MACH_REG_xAX);

		if (m - a > 0) {
			/* open-coded "jb" */
			emit(buf, 0x0f);
			emit(buf, 0x82);

			/* placeholder address */
			jb_addr = buffer_current(buf);
			emit_imm32(buf, 0);
		}

		if (b - m > 0) {
			/* open-coded "ja" */
			emit(buf, 0x0f);
			emit(buf, 0x87);

			/* placeholder address */
			ja_addr = buffer_current(buf);
			emit_imm32(buf, 0);
		}
	}

#ifndef NDEBUG
	/* Make sure what we wanted is what we got;
	 *
	 *     cmp i_method, %eax
	 *     je .okay
	 *     jmp itable_resolver_stub_error
	 * .okay:
	 *
	 */
	__emit_cmp_imm_reg(buf, 1, (long) table[m]->i_method, MACH_REG_xAX);

	/* open-coded "je" */
	emit(buf, 0x0f);
	emit(buf, 0x84);

	uint8_t *je_addr = buffer_current(buf);
	emit_imm32(buf, 0);

	__emit_jmp(buf, (unsigned long) &itable_resolver_stub_error);

	fixup_branch_target(je_addr, buffer_current(buf));
#endif

	__emit_add_imm_reg(buf, sizeof(void *) * table[m]->c_method->virtual_index, MACH_REG_xCX);
	emit_really_indirect_jump_reg(buf, MACH_REG_xCX);

	/* This emits the code for checking the interval [a, m> */
	if (jb_addr) {
		fixup_branch_target(jb_addr, buffer_current(buf));
		emit_itable_bsearch(buf, table, a, m - 1);
	}

	/* This emits the code for checking the interval <m, b] */
	if (ja_addr) {
		fixup_branch_target(ja_addr, buffer_current(buf));
		emit_itable_bsearch(buf, table, m + 1, b);
	}
}

/* Note: table is always sorted on entry->method address */
/* Note: nr_entries is always >= 2 */
void *emit_itable_resolver_stub(struct vm_class *vmc,
	struct itable_entry **table, unsigned int nr_entries)
{
	static struct buffer_operations exec_buf_ops = {
		.expand = NULL,
		.free   = NULL,
	};

	struct buffer *buf = __alloc_buffer(&exec_buf_ops);

	jit_text_lock();

	buf->buf = jit_text_ptr();

	/* Note: When the stub is called, %eax contains the signature hash that
	 * we look up in the stub. 0(%esp) contains the object reference. %ecx
	 * and %edx are available here because they are already saved by the
	 * caller (guaranteed by ABI). */

	/* Load the start of the vtable into %ecx. Later we just add the
	 * right offset to %ecx and jump to *(%ecx). */
	__emit_mov_imm_reg(buf, (long) vmc->vtable.native_ptr, MACH_REG_xCX);

	emit_itable_bsearch(buf, table, 0, nr_entries - 1);

	jit_text_reserve(buffer_offset(buf));
	jit_text_unlock();

	return buffer_ptr(buf);
}


typedef void (*emit_fn_t)(struct insn *insn, struct buffer *, struct basic_block *bb);

#define DECL_EMITTER(_insn_type, _fn) [_insn_type] = _fn

static emit_fn_t emitters[] = {
	DECL_EMITTER(INSN_ADDSD_MEMDISP_XMM, insn_encode),
	DECL_EMITTER(INSN_ADDSD_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_ADDSS_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_ADD_IMM_REG, insn_encode),
	DECL_EMITTER(INSN_ADD_REG_REG, insn_encode),
	DECL_EMITTER(INSN_AND_REG_REG, insn_encode),
	DECL_EMITTER(INSN_CALL_REG, insn_encode),
	DECL_EMITTER(INSN_CALL_REL, emit_call),
	DECL_EMITTER(INSN_CLTD_REG_REG, insn_encode),
	DECL_EMITTER(INSN_DIVSD_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_DIVSS_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_FLD_64_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_FLD_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_FSTP_64_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_FSTP_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_JE_BRANCH, emit_je_branch),
	DECL_EMITTER(INSN_JGE_BRANCH, emit_jge_branch),
	DECL_EMITTER(INSN_JG_BRANCH, emit_jg_branch),
	DECL_EMITTER(INSN_JLE_BRANCH, emit_jle_branch),
	DECL_EMITTER(INSN_JL_BRANCH, emit_jl_branch),
	DECL_EMITTER(INSN_JMP_BRANCH, emit_jmp_branch),
	DECL_EMITTER(INSN_JMP_MEMBASE, insn_encode),
	DECL_EMITTER(INSN_JMP_MEMINDEX, insn_encode),
	DECL_EMITTER(INSN_JNE_BRANCH, emit_jne_branch),
	DECL_EMITTER(INSN_MOVSD_MEMBASE_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSD_MEMDISP_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSD_MEMLOCAL_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSD_XMM_MEMBASE, insn_encode),
	DECL_EMITTER(INSN_MOVSD_XMM_MEMDISP, insn_encode),
	DECL_EMITTER(INSN_MOVSD_XMM_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_MOVSD_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSS_MEMBASE_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSS_MEMDISP_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSS_MEMLOCAL_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSS_XMM_MEMBASE, insn_encode),
	DECL_EMITTER(INSN_MOVSS_XMM_MEMDISP, insn_encode),
	DECL_EMITTER(INSN_MOVSS_XMM_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_MOVSS_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_MOVSXD_REG_REG, insn_encode),
	DECL_EMITTER(INSN_MOVSX_16_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_MOVSX_16_REG_REG, insn_encode),
	DECL_EMITTER(INSN_MOVSX_8_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_MOVSX_8_REG_REG, insn_encode),
	DECL_EMITTER(INSN_MOVZX_16_REG_REG, insn_encode),
	DECL_EMITTER(INSN_MOV_IMM_MEMBASE, emit_mov_imm_membase),
	DECL_EMITTER(INSN_MOV_IMM_MEMLOCAL, emit_mov_imm_memlocal),
	DECL_EMITTER(INSN_MOV_IMM_REG, emit_mov_imm_reg),
	DECL_EMITTER(INSN_MOV_REG_REG, insn_encode),
	DECL_EMITTER(INSN_MULSD_MEMDISP_XMM, insn_encode),
	DECL_EMITTER(INSN_MULSD_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_MULSS_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_NEG_REG, insn_encode),
	DECL_EMITTER(INSN_NOP, insn_encode),
	DECL_EMITTER(INSN_OR_REG_REG, insn_encode),
	DECL_EMITTER(INSN_POP_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_POP_REG, insn_encode),
	DECL_EMITTER(INSN_PUSH_MEMLOCAL, insn_encode),
	DECL_EMITTER(INSN_PUSH_REG, insn_encode),
	DECL_EMITTER(INSN_RET, insn_encode),
	DECL_EMITTER(INSN_SAR_IMM_REG, insn_encode),
	DECL_EMITTER(INSN_SAR_REG_REG, insn_encode),
	DECL_EMITTER(INSN_SHL_REG_REG, insn_encode),
	DECL_EMITTER(INSN_SHR_REG_REG, insn_encode),
	DECL_EMITTER(INSN_SUBSD_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_SUBSS_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_SUB_IMM_REG, insn_encode),
	DECL_EMITTER(INSN_SUB_REG_REG, insn_encode),
	DECL_EMITTER(INSN_XORPD_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_XORPS_XMM_XMM, insn_encode),
	DECL_EMITTER(INSN_XOR_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_XOR_REG_REG, insn_encode),
	DECL_EMITTER(INSN_ADC_IMM_REG, insn_encode),
	DECL_EMITTER(INSN_ADC_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_ADC_REG_REG, insn_encode),
	DECL_EMITTER(INSN_ADD_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_AND_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_CMP_IMM_REG, insn_encode),
	DECL_EMITTER(INSN_CMP_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_CMP_REG_REG, insn_encode),
	DECL_EMITTER(INSN_CONV_FPU64_TO_GPR, emit_conv_fpu64_to_gpr),
	DECL_EMITTER(INSN_CONV_FPU_TO_GPR, emit_conv_fpu_to_gpr),
	DECL_EMITTER(INSN_CONV_GPR_TO_FPU, emit_conv_gpr_to_fpu),
	DECL_EMITTER(INSN_CONV_GPR_TO_FPU64, emit_conv_gpr_to_fpu64),
	DECL_EMITTER(INSN_CONV_XMM64_TO_XMM, emit_conv_xmm64_to_xmm),
	DECL_EMITTER(INSN_CONV_XMM_TO_XMM64, emit_conv_xmm_to_xmm64),
	DECL_EMITTER(INSN_DIV_MEMBASE_REG, emit_div_membase_reg),
	DECL_EMITTER(INSN_DIV_REG_REG, emit_div_reg_reg),
	DECL_EMITTER(INSN_FILD_64_MEMBASE, emit_fild_64_membase),
	DECL_EMITTER(INSN_FISTP_64_MEMBASE, emit_fistp_64_membase),
	DECL_EMITTER(INSN_FLDCW_MEMBASE, emit_fldcw_membase),
	DECL_EMITTER(INSN_FLD_64_MEMBASE, emit_fld_64_membase),
	DECL_EMITTER(INSN_FLD_MEMBASE, emit_fld_membase),
	DECL_EMITTER(INSN_FNSTCW_MEMBASE, emit_fnstcw_membase),
	DECL_EMITTER(INSN_FSTP_64_MEMBASE, emit_fstp_64_membase),
	DECL_EMITTER(INSN_FSTP_MEMBASE, emit_fstp_membase),
	DECL_EMITTER(INSN_MOVSD_MEMINDEX_XMM, emit_mov_64_memindex_xmm),
	DECL_EMITTER(INSN_MOVSD_XMM_MEMINDEX, emit_mov_64_xmm_memindex),
	DECL_EMITTER(INSN_MOVSS_MEMINDEX_XMM, emit_mov_memindex_xmm),
	DECL_EMITTER(INSN_MOVSS_XMM_MEMINDEX, emit_mov_xmm_memindex),
	DECL_EMITTER(INSN_MOV_IMM_THREAD_LOCAL_MEMBASE, emit_mov_imm_thread_local_membase),
	DECL_EMITTER(INSN_MOV_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_MOV_MEMDISP_REG, emit_mov_memdisp_reg),
	DECL_EMITTER(INSN_MOV_MEMINDEX_REG, emit_mov_memindex_reg),
	DECL_EMITTER(INSN_MOV_MEMLOCAL_REG, emit_mov_memlocal_reg),
	DECL_EMITTER(INSN_MOV_REG_MEMBASE, insn_encode),
	DECL_EMITTER(INSN_MOV_REG_MEMDISP, emit_mov_reg_memdisp),
	DECL_EMITTER(INSN_MOV_REG_MEMINDEX, emit_mov_reg_memindex),
	DECL_EMITTER(INSN_MOV_REG_MEMLOCAL, emit_mov_reg_memlocal),
	DECL_EMITTER(INSN_MOV_REG_THREAD_LOCAL_MEMBASE, emit_mov_reg_thread_local_membase),
	DECL_EMITTER(INSN_MOV_REG_THREAD_LOCAL_MEMDISP, emit_mov_reg_thread_local_memdisp),
	DECL_EMITTER(INSN_MOV_THREAD_LOCAL_MEMDISP_REG, emit_mov_thread_local_memdisp_reg),
	DECL_EMITTER(INSN_MUL_MEMBASE_EAX, emit_mul_membase_eax),
	DECL_EMITTER(INSN_MUL_REG_EAX, emit_mul_reg_eax),
	DECL_EMITTER(INSN_MUL_REG_REG, emit_mul_reg_reg),
	DECL_EMITTER(INSN_OR_IMM_MEMBASE, emit_or_imm_membase),
	DECL_EMITTER(INSN_OR_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_PUSH_IMM, emit_push_imm),
	DECL_EMITTER(INSN_SBB_IMM_REG, insn_encode),
	DECL_EMITTER(INSN_SBB_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_SBB_REG_REG, insn_encode),
	DECL_EMITTER(INSN_SUB_MEMBASE_REG, insn_encode),
	DECL_EMITTER(INSN_TEST_IMM_MEMDISP, emit_test_imm_memdisp),
	DECL_EMITTER(INSN_TEST_MEMBASE_REG, insn_encode),
};

static void __emit_insn(struct buffer *buf, struct basic_block *bb, struct insn *insn)
{
	emit_fn_t fn;

	fn = emitters[insn->type];

	if (!fn)
		die("no emitter for instruction type %d", insn->type);

	fn(insn, buf, bb);
}

void emit_insn(struct buffer *buf, struct basic_block *bb, struct insn *insn)
{
	insn->mach_offset = buffer_offset(buf);

	__emit_insn(buf, bb, insn);
}

void emit_nop(struct buffer *buf)
{
	emit(buf, 0x90);
}
