#ifndef JIT_INLINE_CACHE_H
#define JIT_INLINE_CACHE_H

#include "arch/instruction.h"
#include "jit/compilation-unit.h"
#include "jit/vars.h"

extern bool opt_ic_enabled;

struct ic_call {
	struct insn *insn;
	struct list_head list_node;
};

bool ic_enabled(void);
int add_ic_call(struct compilation_unit *cu, struct insn *insn);

#endif
