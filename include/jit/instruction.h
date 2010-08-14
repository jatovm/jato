#ifndef JATO_JIT_INSTRUCTION_H
#define JATO_JIT_INSTRUCTION_H

#include "arch/instruction.h"
#include "lib/list.h"

static inline struct insn *next_insn(struct insn *insn)
{
	return list_entry(insn->insn_list_node.next, struct insn, insn_list_node);
}

static inline struct insn *prev_insn(struct insn *insn)
{
	return list_entry(insn->insn_list_node.prev, struct insn, insn_list_node);
}

struct insn *alloc_insn(enum insn_type);
void free_insn(struct insn *);

int insn_defs(struct compilation_unit *, struct insn *, struct var_info **);
int insn_uses(struct insn *, struct var_info **);
int insn_operand_use_kind(struct insn *, struct operand *operand);

#define for_each_insn(insn, insn_list) list_for_each_entry(insn, insn_list, insn_list_node)

#define for_each_insn_reverse(insn, insn_list) list_for_each_entry_reverse(insn, insn_list, insn_list_node)

#endif /* JATO_JIT_INSTRUCTION_H */
