#ifndef __JIT_INSN_SELECTOR_H
#define __JIT_INSN_SELECTOR_H

struct basic_block;
struct expression;

void insn_select(struct basic_block *, struct expression *);

#endif
