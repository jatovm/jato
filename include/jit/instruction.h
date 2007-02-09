#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <vm/list.h>
#include <stdbool.h>

struct basic_block;

enum machine_reg {
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_EBP,
	REG_ESP,
};

struct var_info {
	enum machine_reg reg;

	struct var_info *next;
};

union operand {
	struct var_info *reg;

	struct {
		struct var_info *base_reg;
		long disp;	/* displacement */
	};

	struct {
		struct var_info *base_reg;
		struct var_info *index_reg;
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
	INSN_CLTD,
	INSN_CMP_IMM_REG,
	INSN_CMP_MEMBASE_REG,
	INSN_DIV_MEMBASE_REG,
	INSN_JE_BRANCH,
	INSN_JNE_BRANCH,
	INSN_JMP_BRANCH,
	INSN_MOV_IMM_MEMBASE,
	INSN_MOV_IMM_REG,
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
	unsigned long offset;
	bool escaped;
};

struct insn *insn(enum insn_type);
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

#define for_each_insn(insn, insn_list) list_for_each_entry(insn, insn_list, insn_list_node)

#endif
