#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <list.h>

enum reg {
	REG_EAX,
	REG_EBP,
};

struct operand {
	union {
		enum reg reg;
		struct {
			enum reg base_reg;
			unsigned long displacement;
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

struct insn *alloc_insn(enum insn_opcode, enum reg, unsigned long, enum reg);
void free_insn(struct insn *);

#endif
