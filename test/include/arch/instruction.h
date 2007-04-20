#ifndef __ARCH_INSTRUCTION_H
#define __ARCH_INSTRUCTION_H

#include <jit/basic-block.h>

#include <stdbool.h>

/*
 * This is MMIX.
 */

enum machine_reg {
	R0,
	R1,
	R2,
};

struct live_range {
	unsigned long start, end;
};

struct var_info {
	unsigned long vreg;
	enum machine_reg reg;
        struct live_range range;
	struct var_info *next;
};

static inline bool is_vreg(struct var_info *var, unsigned long vreg)
{
	return var->vreg == vreg;
}

enum insn_type {
	INSN_ADD,
	INSN_JMP,
	INSN_SETL,
};

union operand {
	struct var_info *reg;
	unsigned long imm;
	struct basic_block *branch_target;
};

struct insn {
	enum insn_type type;
        union {
                struct {
                        union operand x, y, z;
                };
                union operand operand;
        };
        struct list_head insn_list_node;
};

struct insn *arithmetic_insn(enum insn_type, struct var_info *, struct var_info *, struct var_info *);
struct insn *imm_insn(enum insn_type, unsigned long, struct var_info *);
struct insn *branch_insn(enum insn_type, struct basic_block *);

struct insn *alloc_insn(enum insn_type);
void free_insn(struct insn *);

bool insn_defs(struct insn *, unsigned long);
bool insn_uses(struct insn *, unsigned long);

#define for_each_insn(insn, insn_list) list_for_each_entry(insn, insn_list, insn_list_node)

#endif /* __ARCH_INSTRUCTION_H */
