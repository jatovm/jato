#ifndef JATO__JIT_SSA_H
#define JATO__JIT_SSA_H

#include "lib/hash-map.h"

/*
 * The work and inserted arrays from the LIR
 * to SSA algorithm initially contain no block.
 * We mark this special block as SSA_INIT_BLOCK.
 */
#define SSA_INIT_BLOCK -1
#define INIT_VAL	0
#define INIT_BC_OFFSET	0

struct changed_var_stack {
	struct list_head changed_var_stack_node;
	unsigned long vreg;
};

#endif
