#ifndef __PPC_INSTRUCTION_H
#define __PPC_INSTRUCTION_H

#include <stdbool.h>
#include <vm/list.h>
#include <arch/registers.h>
#include <jit/use-position.h>

struct var_info;

/*
 *	Instruction type identifies the opcode, number of operands, and
 * 	operand types.
 */
enum insn_type {
	INSN_BLR,
};

struct insn {
	enum insn_type		type;
	struct list_head	insn_list_node;
	unsigned long		lir_pos;
	unsigned long		bytecode_offset;
	unsigned long		mach_offset;
};

#define for_each_insn(insn, insn_list) list_for_each_entry(insn, insn_list, insn_list_node)

void free_insn(struct insn *);

static inline unsigned long lir_position(struct use_position *reg)
{
	assert(!"oops");
}

static inline bool insn_defs(struct insn *insn, struct var_info *var)
{
	return false;
}

static inline bool insn_uses(struct insn *insn, struct var_info *var)
{
	return false;
}

static inline const char *reg_name(enum machine_reg reg)
{
	return "<unknown>";
}

/*
 * These functions are used by generic code to insert spill/reload
 * instructions.
 */

static inline struct insn *
spill_insn(struct var_info *var, struct stack_slot *slot)
{
	return NULL;
}

static inline struct insn *
reload_insn(struct stack_slot *slot, struct var_info *var)
{
	return NULL;
}

static inline struct insn *
exception_spill_insn(struct stack_slot *slot)
{
	return NULL;
}

#endif /* __PPC_INSTRUCTION_H */
