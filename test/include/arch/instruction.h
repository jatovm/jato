#ifndef __ARCH_INSTRUCTION_H
#define __ARCH_INSTRUCTION_H

/*
 * This is MMIX.
 */

enum machine_reg {
	R0,
	R1,
	R2,
};

struct var_info {
	unsigned long vreg;
	enum machine_reg reg;
	struct var_info *next;
};

struct insn {
        struct list_head insn_list_node;
};

static inline void free_insn(struct insn *insn)
{
}

#endif /* __ARCH_INSTRUCTION_H */
