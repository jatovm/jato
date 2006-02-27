#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <list.h>

enum reg {
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_EBP,
};

struct operand {
	union {
		enum reg reg;
		struct {
			enum reg base_reg;
			unsigned long disp;	/* displacement */
		};
	};
};

enum insn_opcode {
	ADD,
	MOV,
};

struct insn {
	enum insn_opcode insn_op;
	struct operand src;
	struct operand dest;
	struct list_head insn_list_node;
};


static inline struct insn *to_insn(struct list_head *head)
{
	return list_entry(head, struct insn, insn_list_node);
}

struct insn *alloc_insn(enum insn_opcode, enum reg, unsigned long, enum reg);
void free_insn(struct insn *);

#endif
