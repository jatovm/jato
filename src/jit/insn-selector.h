#ifndef __JIT_INSN_SELECTOR_H
#define __JIT_INSN_SELECTOR_H

struct instruction;
struct expression;

struct insn *insn_select(struct expression *);

#endif
