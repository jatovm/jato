#ifndef JATO__JIT_SSA_H
#define JATO__JIT_SSA_H

#include "lib/hash-map.h"

/*
 * The work and inserted arrays from the LIR
 * to SSA algorithm initially contain no block.
 * We mark this special block as SSA_INIT_BLOCK.
 */
#define SSA_INIT_BLOCK -1

struct changed_var_stack {
	struct list_head changed_var_stack_node;
	unsigned long vreg;
};

struct dce {
	struct var_info *var;
	struct list_head dce_node;
};

/*
 * Functions defined in jit/ssa.c
 */
void recompute_insn_positions(struct compilation_unit *);

/*
 * Functions defined in jit/liveness.c
 */

int init_sets(struct compilation_unit *);
void analyze_use_def(struct compilation_unit *);
int analyze_live_sets(struct compilation_unit *);

#endif
