#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <arch/registers.h>
#include <arch/stack-frame.h>
#include <jit/vars.h>
#include <vm/list.h>

#include <assert.h>
#include <stdbool.h>

struct basic_block;
struct bitset;

union operand {
	struct register_info reg;

	struct {
		struct register_info base_reg;
		long disp;	/* displacement */
	};

	struct stack_slot *slot; /* EBP + displacement */

	struct {
		struct register_info base_reg;
		struct register_info index_reg;
		unsigned char shift;
	};

	unsigned long imm;

	unsigned long rel;

	struct basic_block *branch_target;
};

/*
 *	Instruction type identifies the opcode, number of operands, and
 * 	operand types.
 */
enum insn_type {
	INSN_ADD_IMM_REG,
	INSN_ADD_MEMBASE_REG,
	INSN_AND_MEMBASE_REG,
	INSN_CALL_REG,
	INSN_CALL_REL,	
	INSN_CLTD_REG_REG,	/* CDQ in Intel manuals*/
	INSN_CMP_IMM_REG,
	INSN_CMP_MEMBASE_REG,
	INSN_DIV_MEMBASE_REG,
	INSN_JE_BRANCH,
	INSN_JNE_BRANCH,
	INSN_JMP_BRANCH,
	INSN_MOV_IMM_MEMBASE,
	INSN_MOV_IMM_REG,
	INSN_MOV_LOCAL_REG,
	INSN_MOV_MEMBASE_REG,
	INSN_MOV_MEMINDEX_REG,
	INSN_MOV_REG_MEMBASE,
	INSN_MOV_REG_MEMINDEX,
	INSN_MOV_REG_REG,
	INSN_MUL_MEMBASE_REG,
	INSN_NEG_REG,
	INSN_OR_MEMBASE_REG,
	INSN_PUSH_IMM,
	INSN_PUSH_REG,
	INSN_SAR_REG_REG,
	INSN_SHL_REG_REG,
	INSN_SHR_REG_REG,
	INSN_SUB_MEMBASE_REG,
	INSN_XOR_MEMBASE_REG,
};

struct insn {
	enum insn_type type;
	union {
		struct {
			union operand src;
			union operand dest;
		};
		union operand operand;
	};
	struct list_head insn_list_node;
	struct list_head branch_list_node;
	/* Offset in machine code.  */
	unsigned long mach_offset;
	/* Position of this instruction in LIR.  */
	unsigned long lir_pos;
	bool escaped;
};

struct insn *insn(enum insn_type);
struct insn *local_reg_insn(enum insn_type, struct stack_slot *, struct var_info *);
struct insn *membase_reg_insn(enum insn_type, struct var_info *, long, struct var_info *);
struct insn *memindex_reg_insn(enum insn_type, struct var_info *, struct var_info *, unsigned char, struct var_info *);
struct insn *reg_membase_insn(enum insn_type, struct var_info *, struct var_info *, long);
struct insn *reg_memindex_insn(enum insn_type, struct var_info *, struct var_info *, struct var_info *, unsigned char);
struct insn *reg_insn(enum insn_type, struct var_info *);
struct insn *reg_reg_insn(enum insn_type, struct var_info *, struct var_info *);
struct insn *imm_reg_insn(enum insn_type, unsigned long, struct var_info *);
struct insn *imm_membase_insn(enum insn_type, unsigned long, struct var_info *, long);
struct insn *imm_insn(enum insn_type, unsigned long);
struct insn *rel_insn(enum insn_type, unsigned long);
struct insn *branch_insn(enum insn_type, struct basic_block *);

struct insn *alloc_insn(enum insn_type);
void free_insn(struct insn *);

bool insn_defs(struct insn *, unsigned long);
bool insn_uses(struct insn *, unsigned long);

static inline const char *reg_name(enum machine_reg reg)
{
	switch (reg) {
	case REG_EAX:
		return "EAX";
	case REG_EBX:
		return "EBX";
	case REG_ECX:
		return "ECX";
	case REG_EDX:
		return "EDX";
	case REG_EBP:
		return "EBP";
	case REG_ESP:
		return "ESP";
	case REG_UNASSIGNED:
		return "<unassigned>";
	};
	assert(!"not reached");
}

#define for_each_insn(insn, insn_list) list_for_each_entry(insn, insn_list, insn_list_node)

#endif
