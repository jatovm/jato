#ifndef __X86_INSTRUCTION_H
#define __X86_INSTRUCTION_H

#include "jit/use-position.h"

#include "arch/stack-frame.h"
#include "arch/registers.h"
#include "arch/init.h"

#include "lib/list.h"
#include "vm/die.h"

#include <stdbool.h>

struct compilation_unit;
struct basic_block;
struct bitset;

enum operand_type {
	OPERAND_NONE,
	OPERAND_BRANCH,
	OPERAND_IMM,
	OPERAND_MEMBASE,
	OPERAND_MEMDISP,
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
	INSN_AND_REG_REG,
	INSN_CALL_REG,
	INSN_CALL_REL,
	INSN_CLTD_REG_REG,	/* CDQ in Intel manuals*/
	INSN_CMP_IMM_REG,
	INSN_CMP_MEMBASE_REG,
	INSN_CMP_REG_REG,
	INSN_DIV_MEMBASE_REG,
	INSN_DIV_REG_REG,
	INSN_FADD_REG_REG,
	INSN_FADD_64_REG_REG,
	INSN_FADD_64_MEMDISP_REG,
	INSN_FMUL_REG_REG,
	INSN_FMUL_64_REG_REG,
	INSN_FMUL_64_MEMDISP_REG,
	INSN_FDIV_REG_REG,
	INSN_FDIV_64_REG_REG,
	INSN_FSUB_REG_REG,
	INSN_FSUB_64_REG_REG,
	INSN_FLD_MEMBASE,
	INSN_FLD_MEMLOCAL,
	INSN_FLD_64_MEMBASE,
	INSN_FLD_64_MEMLOCAL,
	INSN_FLDCW_MEMBASE,
	INSN_FILD_64_MEMBASE,
	INSN_FISTP_64_MEMBASE,
	INSN_FNSTCW_MEMBASE,
	INSN_FSTP_MEMBASE,
	INSN_FSTP_MEMLOCAL,
	INSN_FSTP_64_MEMBASE,
	INSN_FSTP_64_MEMLOCAL,
	INSN_CONV_FPU_TO_GPR,
	INSN_CONV_FPU64_TO_GPR,
	INSN_CONV_GPR_TO_FPU,
	INSN_CONV_GPR_TO_FPU64,
	INSN_CONV_XMM_TO_XMM64,
	INSN_CONV_XMM64_TO_XMM,
	INSN_JE_BRANCH,
	INSN_JGE_BRANCH,
	INSN_JG_BRANCH,
	INSN_JLE_BRANCH,
	INSN_JL_BRANCH,
	INSN_JMP_MEMINDEX,
	INSN_JMP_MEMBASE,
	INSN_JMP_BRANCH,
	INSN_JNE_BRANCH,
	INSN_MOV_IMM_MEMBASE,
	INSN_MOV_IMM_MEMLOCAL,
	INSN_MOV_IMM_REG,
	INSN_MOV_IMM_THREAD_LOCAL_MEMBASE,
	INSN_MOV_IP_REG,
	INSN_MOV_MEMLOCAL_REG,
	INSN_MOV_MEMLOCAL_XMM,
	INSN_MOV_64_MEMLOCAL_XMM,
	INSN_MOV_MEMBASE_REG,
	INSN_MOV_MEMDISP_REG,
	INSN_MOV_MEMDISP_XMM,
	INSN_MOV_64_MEMDISP_XMM,
	INSN_MOV_REG_MEMDISP,
	INSN_MOV_REG_THREAD_LOCAL_MEMBASE,
	INSN_MOV_REG_THREAD_LOCAL_MEMDISP,
	INSN_MOV_THREAD_LOCAL_MEMDISP_REG,
	INSN_MOV_MEMINDEX_REG,
	INSN_MOV_REG_MEMBASE,
	INSN_MOV_REG_MEMINDEX,
	INSN_MOV_REG_MEMLOCAL,
	INSN_MOV_XMM_MEMLOCAL,
	INSN_MOV_64_XMM_MEMLOCAL,
	INSN_MOV_REG_REG,
	INSN_MOV_MEMBASE_XMM,
	INSN_MOV_64_MEMBASE_XMM,
	INSN_MOV_MEMINDEX_XMM,
	INSN_MOV_64_MEMINDEX_XMM,
	INSN_MOV_XMM_MEMBASE,
	INSN_MOV_64_XMM_MEMBASE,
	INSN_MOV_XMM_MEMDISP,
	INSN_MOV_64_XMM_MEMDISP,
	INSN_MOV_XMM_MEMINDEX,
	INSN_MOV_64_XMM_MEMINDEX,
	INSN_MOV_XMM_XMM,
	INSN_MOV_64_XMM_XMM,
	INSN_MOVSX_8_REG_REG,
	INSN_MOVSX_8_MEMBASE_REG,
	INSN_MOVSX_16_REG_REG,
	INSN_MOVSX_16_MEMBASE_REG,
	INSN_MOVZX_16_REG_REG,
	INSN_MUL_MEMBASE_EAX,
	INSN_MUL_REG_EAX,
	INSN_MUL_REG_REG,
	INSN_NEG_REG,
	INSN_OR_IMM_MEMBASE,
	INSN_OR_MEMBASE_REG,
	INSN_OR_REG_REG,
	INSN_PUSH_IMM,
	INSN_PUSH_REG,
	INSN_PUSH_MEMLOCAL,
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
	INSN_TEST_IMM_MEMDISP,
	INSN_TEST_MEMBASE_REG,
	INSN_XOR_MEMBASE_REG,
	INSN_XOR_IMM_REG,
	INSN_XOR_REG_REG,
	INSN_XOR_XMM_REG_REG,
	INSN_XOR_64_XMM_REG_REG,

	/* Must be last */
	NR_INSN_TYPES,
};

enum insn_flag_type {
	INSN_FLAG_ESCAPED		= 1U << 0,
	INSN_FLAG_SAFEPOINT		= 1U << 1,
	INSN_FLAG_KNOWN_BC_OFFSET	= 1U << 2,
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
		struct operand operands[2];
		struct {
			struct operand src;
			struct operand dest;
		};
		struct operand operand;
	};
};

/* Each instruction has two operands and each of them can refer to two explicit
   registers.  An instruction can also clobber EAX/RAX, ECX/RCX, and EDX/RDX
   implicitly.  */
#define MAX_REG_OPERANDS (4 + 3)

void insn_sanity_check(void);

struct insn *insn(enum insn_type);
struct insn *memlocal_reg_insn(enum insn_type, struct stack_slot *, struct var_info *);
struct insn *membase_reg_insn(enum insn_type, struct var_info *, long, struct var_info *);
struct insn *memindex_insn(enum insn_type, struct var_info *, struct var_info *, unsigned char);
struct insn *memindex_reg_insn(enum insn_type, struct var_info *, struct var_info *, unsigned char, struct var_info *);
struct insn *reg_memindex_insn(enum insn_type, struct var_info *, struct var_info *, struct var_info *, unsigned char);
struct insn *reg_membase_insn(enum insn_type, struct var_info *, struct var_info *, long);
struct insn *reg_memlocal_insn(enum insn_type, struct var_info *, struct stack_slot *);
struct insn *reg_insn(enum insn_type, struct var_info *);
struct insn *reg_reg_insn(enum insn_type, struct var_info *, struct var_info *);
struct insn *imm_reg_insn(enum insn_type, unsigned long, struct var_info *);
struct insn *memdisp_reg_insn(enum insn_type, unsigned long, struct var_info *);
struct insn *reg_memdisp_insn(enum insn_type, struct var_info *, unsigned long);
struct insn *imm_memdisp_insn(enum insn_type, long, long);
struct insn *imm_membase_insn(enum insn_type, unsigned long, struct var_info *, long);
struct insn *imm_memlocal_insn(enum insn_type, unsigned long, struct stack_slot *);
struct insn *imm_insn(enum insn_type, unsigned long);
struct insn *rel_insn(enum insn_type, unsigned long);
struct insn *branch_insn(enum insn_type, struct basic_block *);
struct insn *memlocal_insn(enum insn_type, struct stack_slot *);
struct insn *membase_insn(enum insn_type, struct var_info *, long);

/*
 * These functions are used by generic code to insert spill/reload
 * instructions.
 */

int insert_copy_slot_32_insns(struct stack_slot *, struct stack_slot *, struct list_head *, unsigned long);
int insert_copy_slot_64_insns(struct stack_slot *, struct stack_slot *, struct list_head *, unsigned long);

struct insn *spill_insn(struct var_info *var, struct stack_slot *slot);
struct insn *reload_insn(struct stack_slot *slot, struct var_info *var);
struct insn *jump_insn(struct basic_block *bb);

bool insn_is_branch(struct insn *insn);
bool insn_is_call(struct insn *insn);

#endif
