#ifndef JIT_INLINE_CACHE_H
#define JIT_INLINE_CACHE_H

#include "arch/instruction.h"
#include "jit/compilation-unit.h"
#include "jit/vars.h"

struct ic_call {
	struct insn *insn;
	struct list_head list_node;
};

int add_ic_call(struct compilation_unit *cu, struct insn *insn);

#endif
