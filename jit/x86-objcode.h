#ifndef __JIT_X86_OBJCODE_H
#define __JIT_X86_OBJCODE_H

#include <instruction.h>

struct basic_block;
struct compilation_unit;

struct insn_sequence {
	unsigned char *start;
	unsigned char *current;
	unsigned char *end;
};

static inline void init_insn_sequence(struct insn_sequence *is, void *code,
				      unsigned long size)
{
	is->start = is->current = code;
	is->end = code + size;
}

void x86_emit_prolog(struct insn_sequence *);
void x86_emit_epilog(struct insn_sequence *);
void x86_emit_obj_code(struct basic_block *, struct insn_sequence *);
void x86_emit_trampoline(struct compilation_unit *, void *, void *, unsigned long);
    
#endif
