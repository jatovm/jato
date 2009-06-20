#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <jit/use-position.h>
#include <arch/registers.h>
#include <arch/stack-frame.h>
#include <vm/list.h>

#include <stdbool.h>

struct basic_block;
struct bitset;

enum operand_type {
	OPERAND_NONE,
	OPERAND_BRANCH,
	OPERAND_IMM,
	OPERAND_MEMBASE,
	OPERAND_MEMINDEX,
	OPERAND_MEMLOCAL,
	OPERAND_REG,
	OPERAND_REL,
	LAST_OPERAND
};

struct operand {
	enum operand_type type;
	union {
		struct use_position reg;

		struct {
			struct use_position base_reg;
			long disp;	/* displacement */
		};

		struct stack_slot *slot; /* EBP + displacement */

		struct {
			struct use_position base_reg;
			struct use_position index_reg;
			unsigned char shift;
		};

		unsigned long imm;

		unsigned long rel;

		struct basic_block *branch_target;
	};
};

/*
 *	Instruction type identifies the opcode, number of operands, and
 * 	operand types.
 */
enum insn_type {
	INSN_ADC_IMM_REG,
	INSN_ADC_MEMBASE_REG,
	INSN_ADC_REG_REG,
	INSN_ADD_IMM_REG,
	INSN_ADD_MEMBASE_REG,
	INSN_ADD_REG_REG,
	INSN_AND_MEMBASE_REG,
	INSN_CALL_REG,
	INSN_CALL_REL,
	INSN_CLTD_REG_REG,	/* CDQ in Intel manuals*/
	INSN_CMP_IMM_REG,
	INSN_CMP_MEMBASE_REG,
	INSN_CMP_REG_REG,
	INSN_DIV_MEMBASE_REG,
	INSN_DIV_REG_REG,
	INSN_JE_BRANCH,
	INSN_JGE_BRANCH,
	INSN_JG_BRANCH,
	INSN_JLE_BRANCH,
	INSN_JL_BRANCH,
	INSN_JMP_BRANCH,
	INSN_JNE_BRANCH,
	INSN_MOV_IMM_MEMBASE,
	INSN_MOV_IMM_REG,
	INSN_MOV_MEMLOCAL_REG,
	INSN_MOV_MEMBASE_REG,
	INSN_MOV_THREAD_LOCAL_MEMDISP_REG,
	INSN_MOV_MEMINDEX_REG,
	INSN_MOV_REG_MEMBASE,
	INSN_MOV_REG_MEMINDEX,
	INSN_MOV_REG_MEMLOCAL,
	INSN_MOV_REG_REG,
	INSN_MUL_MEMBASE_EAX,
	INSN_MUL_REG_EAX,
	INSN_MUL_REG_REG,
	INSN_NEG_REG,
	INSN_OR_MEMBASE_REG,
	INSN_OR_REG_REG,
	INSN_PUSH_IMM,
	INSN_PUSH_REG,
	INSN_POP_MEMLOCAL,
	INSN_POP_REG,
	INSN_RET,
	INSN_SAR_IMM_REG,
	INSN_SAR_REG_REG,
	INSN_SBB_IMM_REG,
	INSN_SBB_MEMBASE_REG,
	INSN_SBB_REG_REG,
	INSN_SHL_REG_REG,
	INSN_SHR_REG_REG,
	INSN_SUB_IMM_REG,
	INSN_SUB_MEMBASE_REG,
	INSN_SUB_REG_REG,
	INSN_TEST_MEMBASE_REG,
	INSN_XOR_MEMBASE_REG,
	INSN_XOR_IMM_REG,

#ifdef CONFIG_X86_64
	INSN64_ADD_IMM_REG,
	INSN64_MOV_REG_REG,
	INSN64_PUSH_IMM,
	INSN64_PUSH_REG,
	INSN64_POP_REG,
	INSN64_SUB_IMM_REG,

	INSN32_ADD_IMM_REG,
	INSN32_MOV_REG_REG,
	INSN32_PUSH_IMM,
	INSN32_PUSH_REG,
	INSN32_POP_REG,
	INSN32_SUB_IMM_REG,

	/* Aliases for instructions in common code. */
	INSN64_CALL_REL		= INSN_CALL_REL,
	INSN64_JE_BRANCH	= INSN_JE_BRANCH,
	INSN64_JGE_BRANCH	= INSN_JGE_BRANCH,
	INSN64_JG_BRANCH	= INSN_JG_BRANCH,
	INSN64_JLE_BRANCH	= INSN_JLE_BRANCH,
	INSN64_JL_BRANCH	= INSN_JL_BRANCH,
	INSN64_JMP_BRANCH	= INSN_JMP_BRANCH,
	INSN64_JNE_BRANCH	= INSN_JNE_BRANCH,
	INSN64_RET		= INSN_RET,
#endif
};

struct insn {
	enum insn_type type;
	union {
		struct operand operands[2];
		struct {
			struct operand src;
			struct operand dest;
		};
		struct operand operand;
	};
	struct list_head insn_list_node;
	struct list_head branch_list_node;
	/* Offset in machine code.  */
	unsigned long mach_offset;
	/* Position of this instruction in LIR.  */
	unsigned long lir_pos;
	bool escaped;
	unsigned long bytecode_offset;
};

static inline unsigned long lir_position(struct use_position *reg)
{
	return reg->insn->lir_pos;
}

struct insn *insn(enum insn_type);
struct insn *memlocal_reg_insn(enum insn_type, struct stack_slot *, struct var_info *);
struct insn *membase_reg_insn(enum insn_type, struct var_info *, long, struct var_info *);
struct insn *memindex_reg_insn(enum insn_type, struct var_info *, struct var_info *, unsigned char, struct var_info *);
struct insn *reg_memindex_insn(enum insn_type, struct var_info *, struct var_info *, struct var_info *, unsigned char);
struct insn *reg_membase_insn(enum insn_type, struct var_info *, struct var_info *, long);
struct insn *reg_memlocal_insn(enum insn_type, struct var_info *, struct stack_slot *);
struct insn *reg_insn(enum insn_type, struct var_info *);
struct insn *reg_reg_insn(enum insn_type, struct var_info *, struct var_info *);
struct insn *imm_reg_insn(enum insn_type, unsigned long, struct var_info *);
struct insn *imm_membase_insn(enum insn_type, unsigned long, struct var_info *, long);
struct insn *imm_insn(enum insn_type, unsigned long);
struct insn *rel_insn(enum insn_type, unsigned long);
struct insn *branch_insn(enum insn_type, struct basic_block *);
struct insn *memlocal_insn(enum insn_type, struct stack_slot *);

/*
 * These functions are used by generic code to insert spill/reload
 * instructions.
 */

static inline struct insn *
spill_insn(struct var_info *var, struct stack_slot *slot)
{
	return reg_memlocal_insn(INSN_MOV_REG_MEMLOCAL, var, slot);
}

static inline struct insn *
reload_insn(struct stack_slot *slot, struct var_info *var)
{
	return memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, slot, var);
}

struct insn *alloc_insn(enum insn_type);
void free_insn(struct insn *);

bool insn_defs(struct insn *, struct var_info *);
bool insn_uses(struct insn *, struct var_info *);

#define for_each_insn(insn, insn_list) list_for_each_entry(insn, insn_list, insn_list_node)

#endif
