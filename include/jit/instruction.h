#ifndef JATO_JIT_INSTRUCTION_H
#define JATO_JIT_INSTRUCTION_H

#include "arch/instruction.h"
#include "lib/list.h"

static inline struct insn *next_insn(struct insn *insn)
{
	return list_entry(insn->insn_list_node.next, struct insn, insn_list_node);
}

#endif /* JATO_JIT_INSTRUCTION_H */
