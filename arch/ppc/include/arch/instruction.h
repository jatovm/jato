#ifndef __PPC_INSTRUCTION_H
#define __PPC_INSTRUCTION_H

#include <stdbool.h>
#include "lib/list.h"
#include "arch/registers.h"
#include "jit/use-position.h"

struct var_info;

/*
 *	Instruction type identifies the opcode, number of operands, and
 * 	operand types.
 */
enum insn_type {
	INSN_BLR,
	INSN_LIS,
	INSN_ORI,
};

enum operand_type {
	OPERAND_REG,
	OPERAND_IMM,

	/* This must be last */
	LAST_OPERAND
};

struct operand {
	enum operand_type		type;
	union {
		struct use_position	reg;
		struct basic_block	*branch_target;
		uint16_t		imm;
	};
};

enum insn_flag_type {
	INSN_FLAG_ESCAPED		= 1U << 0,
	INSN_FLAG_SAFEPOINT		= 1U << 1,
	INSN_FLAG_KNOWN_BC_OFFSET	= 1U << 2,
	INSN_FLAG_RENAMED		= 1U << 3, /* instruction with renamed virtual registers */
	INSN_FLAG_SSA_ADDED		= 1U << 4, /* instruction added during SSA deconstruction */
};

struct insn {
	uint8_t			type;		 /* see enum insn_type */
	uint8_t			flags;		 /* see enum insn_flag_type */
	uint16_t		bc_offset;	 /* offset in bytecode */
	uint32_t		mach_offset;	 /* offset in machine code */
	uint32_t		lir_pos;	 /* offset in LIR */
	struct list_head	insn_list_node;
	struct list_head	branch_list_node;

	union {
		struct operand operands[3];
		struct {
			struct operand src;
			struct operand dest;
		};
		struct {
			struct operand *ssa_srcs;
			struct operand ssa_dest;
			unsigned long nr_srcs;
		};
		struct operand operand;
	};
};

#define MAX_REG_OPERANDS 2

void insn_sanity_check(void);

struct insn *insn(enum insn_type);

/*
 * These functions are used by generic code to insert spill/reload
 * instructions.
 */

int insert_copy_slot_32_insns(struct stack_slot *, struct stack_slot *,
					struct list_head *, unsigned long);
int insert_copy_slot_64_insns(struct stack_slot *, struct stack_slot *,
					struct list_head *, unsigned long);

struct insn *spill_insn(struct var_info *var, struct stack_slot *slot);
struct insn *reload_insn(struct stack_slot *slot, struct var_info *var);
struct insn *jump_insn(struct basic_block *bb);

bool insn_is_branch(struct insn *insn);
bool insn_is_jmp_branch(struct insn *insn);
bool insn_is_call(struct insn *insn);

int ssa_modify_insn_type(struct insn *);
void imm_operand(struct operand *, unsigned long);

#endif /* __PPC_INSTRUCTION_H */
