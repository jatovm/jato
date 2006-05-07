#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <vm/list.h>

enum reg {
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_EBP,
	REG_ESP,
};

struct operand {
	union {
		enum reg reg;
		struct {
			enum reg base_reg;
			unsigned long disp;	/* displacement */
		};
		unsigned long imm;
	};
};

enum insn_opcode {
	INSN_ADD,
	INSN_MOV,
	INSN_CALL,
	INSN_PUSH,
};

struct insn {
	enum insn_opcode insn_op;
	union {
		struct {
			struct operand src;
			struct operand dest;
		};
		struct operand operand;
	};
	struct list_head insn_list_node;
};


static inline struct insn *insn_entry(struct list_head *head)
{
	return list_entry(head, struct insn, insn_list_node);
}

struct insn *alloc_insn(enum insn_opcode);
struct insn *x86_op_membase_reg(enum insn_opcode, enum reg, unsigned long, enum reg);
void free_insn(struct insn *);

#endif
