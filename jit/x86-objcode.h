#ifndef __JIT_ASSEMBLER_H
#define __JIT_ASSEMBLER_H

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
void x86_emit_obj_code(struct basic_block *, struct insn_sequence *);

#endif
