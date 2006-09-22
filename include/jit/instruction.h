#ifndef __JIT_INSTRUCTION_H
#define __JIT_INSTRUCTION_H

#include <vm/list.h>

struct basic_block;

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
		/* OPERAND_REGISTER */
		enum reg reg;

		/* OPERAND_MEMBASE */
		struct {
			enum reg base_reg;
			long disp;	/* displacement */
		};

		/* OPERAND_IMMEDIATE */
		unsigned long imm;

		/* OPERAND_RELATIVE */
		unsigned long rel;

		/* OPERAND_BRANCH */
		struct basic_block *branch_target;
	};
};

enum insn_opcode {
	OPC_ADD,
	OPC_CALL,
	OPC_CLTD,
	OPC_CMP,
	OPC_DIV,
	OPC_JE,
	OPC_JMP,
	OPC_MOV,
	OPC_MUL,
	OPC_NEG,
	OPC_PUSH,
	OPC_SHL,
	OPC_SAR,
	OPC_SHR,
	OPC_SUB,
};

enum operand_type {
	OPERAND_RELATIVE,	/* Call target */
	OPERAND_REGISTER,	/* Register */
	OPERAND_IMMEDIATE,	/* Immediate value */
	OPERAND_MEMBASE,	/* Memory operand consisting of base offset
				   in register with immediate value
				   displacement.  */
	OPERAND_BRANCH,		/* Branch target */
};

#define OPERAND_TYPE_SHIFT 8UL

#define SRC_OPERAND_TYPE_SHIFT OPERAND_TYPE_SHIFT
#define DEST_OPERAND_TYPE_SHIFT 16UL

/*	Instruction that has no operands.  */
#define DEFINE_INSN_TYPE_0(opc) (opc)

/*	Instruction that operates on one operand.  */
#define DEFINE_INSN_TYPE_1(opc, operand_type) (((operand_type) << OPERAND_TYPE_SHIFT) | (opc))

/*	Instruction that operates on two operands. */
#define DEFINE_INSN_TYPE_2(opc, src_operand_type, dest_operand_type) \
	(((src_operand_type) << SRC_OPERAND_TYPE_SHIFT) |((dest_operand_type) << DEST_OPERAND_TYPE_SHIFT) | (opc))

/*
 *	Instruction type identifies the opcode, number of operands, and
 * 	operand types.
 */
enum insn_type {
	INSN_ADD_IMM_REG	= DEFINE_INSN_TYPE_2(OPC_ADD,  OPERAND_IMMEDIATE, OPERAND_REGISTER),
	INSN_ADD_MEMBASE_REG	= DEFINE_INSN_TYPE_2(OPC_ADD,  OPERAND_MEMBASE,   OPERAND_REGISTER),
	INSN_CALL_REG		= DEFINE_INSN_TYPE_1(OPC_CALL, OPERAND_REGISTER),
	INSN_CALL_REL		= DEFINE_INSN_TYPE_1(OPC_CALL, OPERAND_RELATIVE),
	INSN_CLTD		= DEFINE_INSN_TYPE_0(OPC_CLTD),
	INSN_CMP_IMM_REG	= DEFINE_INSN_TYPE_2(OPC_CMP,  OPERAND_IMMEDIATE, OPERAND_REGISTER),
	INSN_CMP_MEMBASE_REG	= DEFINE_INSN_TYPE_2(OPC_CMP,  OPERAND_MEMBASE,   OPERAND_REGISTER),
	INSN_DIV_MEMBASE_REG	= DEFINE_INSN_TYPE_2(OPC_DIV,  OPERAND_MEMBASE,   OPERAND_REGISTER),
	INSN_JE_BRANCH		= DEFINE_INSN_TYPE_1(OPC_JE,   OPERAND_BRANCH),
	INSN_JMP_BRANCH		= DEFINE_INSN_TYPE_1(OPC_JMP,  OPERAND_BRANCH),
	INSN_MOV_IMM_MEMBASE	= DEFINE_INSN_TYPE_2(OPC_MOV,  OPERAND_IMMEDIATE, OPERAND_MEMBASE),
	INSN_MOV_IMM_REG	= DEFINE_INSN_TYPE_2(OPC_MOV,  OPERAND_IMMEDIATE, OPERAND_REGISTER),
	INSN_MOV_MEMBASE_REG	= DEFINE_INSN_TYPE_2(OPC_MOV,  OPERAND_MEMBASE,   OPERAND_REGISTER),
	INSN_MOV_REG_MEMBASE    = DEFINE_INSN_TYPE_2(OPC_MOV,  OPERAND_REGISTER,  OPERAND_MEMBASE),
	INSN_MOV_REG_REG	= DEFINE_INSN_TYPE_2(OPC_MOV,  OPERAND_REGISTER,  OPERAND_REGISTER),
	INSN_MUL_MEMBASE_REG	= DEFINE_INSN_TYPE_2(OPC_MUL,  OPERAND_MEMBASE,   OPERAND_REGISTER),
	INSN_NEG_REG		= DEFINE_INSN_TYPE_1(OPC_NEG,  OPERAND_REGISTER),
	INSN_PUSH_IMM		= DEFINE_INSN_TYPE_1(OPC_PUSH, OPERAND_IMMEDIATE),
	INSN_PUSH_REG		= DEFINE_INSN_TYPE_1(OPC_PUSH, OPERAND_REGISTER),
	INSN_SAR_REG_REG	= DEFINE_INSN_TYPE_2(OPC_SAR,  OPERAND_REGISTER,  OPERAND_REGISTER),
	INSN_SHL_REG_REG	= DEFINE_INSN_TYPE_2(OPC_SHL,  OPERAND_REGISTER,  OPERAND_REGISTER),
	INSN_SHR_REG_REG	= DEFINE_INSN_TYPE_2(OPC_SHR,  OPERAND_REGISTER,  OPERAND_REGISTER),
	INSN_SUB_MEMBASE_REG	= DEFINE_INSN_TYPE_2(OPC_SUB,  OPERAND_MEMBASE,   OPERAND_REGISTER),
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
	struct list_head insn_list_node;
	struct list_head branch_list_node;
	unsigned long offset;
};

struct insn *insn(enum insn_opcode);
struct insn *membase_reg_insn(enum insn_opcode, enum reg, long, enum reg);
struct insn *reg_membase_insn(enum insn_opcode, enum reg, enum reg, long);
struct insn *reg_insn(enum insn_opcode, enum reg);
struct insn *reg_reg_insn(enum insn_opcode, enum reg, enum reg);
struct insn *imm_reg_insn(enum insn_opcode, unsigned long, enum reg);
struct insn *imm_membase_insn(enum insn_opcode, unsigned long, enum reg, long);
struct insn *imm_insn(enum insn_opcode, unsigned long);
struct insn *rel_insn(enum insn_opcode, unsigned long);
struct insn *branch_insn(enum insn_opcode, struct basic_block *);

struct insn *alloc_insn(enum insn_type);
void free_insn(struct insn *);

#endif
