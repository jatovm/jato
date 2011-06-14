#ifndef JATO__JIT_SSA_H
#define JATO__JIT_SSA_H

/*
 * The work and inserted arrays from the LIR
 * to SSA algorithm initially contain no block.
 * We mark this special block as SSA_INIT_BLOCK.
 */
#define SSA_INIT_BLOCK -1

struct insn_add_ons {
	struct use_position reg;
	struct list_head insn_list_node;
};

struct changed_var_stack {
	struct list_head changed_var_stack_node;
	unsigned long vreg;
};

struct use_position *get_reg_from_insn(struct insn* insn);
bool bb_is_eh(struct compilation_unit *, struct basic_block *);

#endif
