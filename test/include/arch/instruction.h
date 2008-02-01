#ifndef __ARCH_INSTRUCTION_H
#define __ARCH_INSTRUCTION_H

#include <arch/registers.h>
#include <jit/basic-block.h>
#include <jit/vars.h>

#include <stdbool.h>

/*
 * This is MMIX.
 */

enum insn_type {
	INSN_ADD,
	INSN_JMP,
	INSN_SETL,
};

union operand {
	struct register_info	reg;
	unsigned long		imm;
	struct basic_block	*branch_target;
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
	/* Position of this instruction in LIR.  */
	unsigned long		lir_pos;
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
