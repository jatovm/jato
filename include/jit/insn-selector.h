#ifndef __JIT_INSN_SELECTOR_H
#define __JIT_INSN_SELECTOR_H

struct basic_block;

int select_instructions(struct compilation_unit *cu);
void insn_select(struct basic_block *);

#endif
