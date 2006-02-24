#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <list.h>

enum insn_opcode {
	ADD,
	MOV,
};

struct insn {
	enum insn_opcode insn_op;
	unsigned long src;
	unsigned long dest;
	struct list_head insns;
};

struct insn *alloc_insn(enum insn_opcode, unsigned long, unsigned long);
void free_insn(struct insn *);

#endif
