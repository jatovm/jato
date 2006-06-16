#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <vm/list.h>

struct statement;

enum reg {
	REG_EAX,
	REG_EBX,
	REG_ECX,
	REG_EDX,
	REG_EBP,
	REG_ESP,
};

struct operand {
	union {
		enum reg reg;
		struct {
			enum reg base_reg;
			unsigned long disp;	/* displacement */
		};
		unsigned long imm;
		unsigned long rel;
	};
};

enum insn_opcode {
	OPC_ADD,
	OPC_CALL,
	OPC_CMP,
	OPC_JE,
	OPC_MOV,
	OPC_PUSH,
};

/*
 *	The type of an operand is called an addressing mode.
 */
enum addressing_mode {
	AM_REL,		/* Relative. Only used as an addressing mode for code.  */
	AM_REG,		/* Register */
	AM_IMM,		/* Immediate */
	AM_DISP,	/* Displacement */
	AM_BRANCH,	/* Branch -- HACK */
};

#define OPERAND_1_AM_SHIFT 8UL
#define OPERAND_2_AM_SHIFT 16UL

/*
 *	1-operand instruction type
 */
#define DEFINE_INSN_TYPE(opc, am) \
	(((am) << OPERAND_1_AM_SHIFT) | (opc))

/*
 *	2-operand instruction type
 */
#define DEFINE_INSN_TYPE_2(opc, am1, am2) \
	(((am1) << OPERAND_2_AM_SHIFT) | ((am2) << OPERAND_1_AM_SHIFT) | (opc))

/*
 *	Instruction type identifies the opcode, number of operands, and
 *	addressing modes of the operands of an instruction.
 */
enum insn_type {
	INSN_ADD_DISP_REG	= DEFINE_INSN_TYPE_2(OPC_ADD, AM_DISP, AM_REG),
	INSN_ADD_IMM_REG	= DEFINE_INSN_TYPE_2(OPC_ADD, AM_IMM, AM_REG),
	INSN_CALL_REL		= DEFINE_INSN_TYPE(OPC_CALL, AM_REL),
	INSN_CMP_DISP_REG	= DEFINE_INSN_TYPE_2(OPC_CMP, AM_DISP, AM_REG),
	INSN_JE_BRANCH		= DEFINE_INSN_TYPE(OPC_JE, AM_BRANCH),
	INSN_MOV_DISP_REG	= DEFINE_INSN_TYPE_2(OPC_MOV, AM_DISP, AM_REG),
	INSN_MOV_IMM_REG	= DEFINE_INSN_TYPE_2(OPC_MOV, AM_IMM, AM_REG),
	INSN_MOV_IMM_DISP	= DEFINE_INSN_TYPE_2(OPC_MOV, AM_IMM, AM_DISP),
	INSN_PUSH_IMM		= DEFINE_INSN_TYPE(OPC_PUSH, AM_IMM),
	INSN_PUSH_REG		= DEFINE_INSN_TYPE(OPC_PUSH, AM_REG),
};

struct insn {
	enum insn_type type;
	union {
		struct {
			struct operand src;
			struct operand dest;
		};
		struct operand operand;
	};
	struct statement *branch_target;
	struct list_head insn_list_node;
	struct list_head branch_list_node;
	unsigned long offset;
};


static inline struct insn *insn_entry(struct list_head *head)
{
	return list_entry(head, struct insn, insn_list_node);
}

struct insn *disp_reg_insn(enum insn_opcode, enum reg, unsigned long, enum reg);
struct insn *reg_insn(enum insn_opcode, enum reg);
struct insn *imm_reg_insn(enum insn_opcode, unsigned long, enum reg);
struct insn *imm_disp_insn(enum insn_opcode, unsigned long, enum reg, unsigned long);
struct insn *imm_insn(enum insn_opcode, unsigned long);
struct insn *rel_insn(enum insn_opcode, unsigned long);
struct insn *branch_insn(enum insn_opcode, struct statement *);

struct insn *alloc_insn(enum insn_type);
void free_insn(struct insn *);

#endif
