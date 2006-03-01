#ifndef __JIT_X86_OBJCODE_H
#define __JIT_X86_OBJCODE_H

#include <instruction.h>

struct basic_block;

struct insn_sequence {
	unsigned char *current, *end;
};

static inline void init_insn_sequence(struct insn_sequence *is, void *code,
				      unsigned long size)
{
	is->current = code;
	is->end = code + size;
}

void x86_emit_prolog(struct insn_sequence *);
void x86_emit_epilog(struct insn_sequence *);
void x86_emit_push_imm32(struct insn_sequence *, int);
void x86_emit_call(struct insn_sequence *, void *);
void x86_emit_ret(struct insn_sequence *);
void x86_emit_add_imm8_reg(struct insn_sequence *, unsigned char, enum reg);
void x86_emit_indirect_jump_reg(struct insn_sequence *, enum reg);
void x86_emit_obj_code(struct basic_block *, struct insn_sequence *);

#endif
