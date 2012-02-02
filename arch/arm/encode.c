#include "arch/encode.h"

#include "arch/instruction.h"
#include "arch/stack-frame.h"

#include "lib/buffer.h"

#include "vm/die.h"

#include <inttypes.h>
#include <stdint.h>

#define INVALID_INSN			0

/*
 * Max value of frame size that can be subtracted from sp in one instruction.
 */
#define MAX_FRAME_SIZE_SUBTRACTED	252

static uint8_t arm_register_numbers[] = {
	[MACH_REG_R0]		= 0x00,
	[MACH_REG_R1]		= 0x01,
	[MACH_REG_R2]		= 0x02,
	[MACH_REG_R3]		= 0x03,
	[MACH_REG_R4]		= 0x04,
	[MACH_REG_R5]		= 0x05,
	[MACH_REG_R6]		= 0x06,
	[MACH_REG_R7]		= 0x07,
	[MACH_REG_R8]		= 0x08,
	[MACH_REG_R9]		= 0x09,
	[MACH_REG_R10]		= 0x0A,
	[MACH_REG_FP]		= 0x0B,
	[MACH_REG_IP]		= 0x0C,
	[MACH_REG_SP]		= 0x0D,
	[MACH_REG_LR]		= 0x0E,
	[MACH_REG_PC]		= 0x0F,
};

/* Encodes the register for arm instructions */
uint8_t arm_encode_reg(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		die("unassigned register during code emission");

	if (reg < 0 || reg >= ARRAY_SIZE(arm_register_numbers))
		die("unknown register %d", reg);

	return arm_register_numbers[reg];
}

/*
 * All ARM instructions are conditionally executed
 * so all of them have a 4bit condition field at start
 */
enum arm_conditions {
	AL	= (0xEUL << 28),	/* Always execute the instruction */
};

/* Encodings for different fields of ARM */
enum arm_fields {
	/* F_field denotes the category of the instruction */
	DATA_PROCESSING			= (0x0UL << 26),
	LOAD_STORE			= (0x1UL << 26),
	BRANCH				= (0x2UL << 26),
	MULTIPLE_LOAD_STORE		= (0x2UL << 26),	/* Load and Store multiple registers */

	/* This field give information about acccessing */
	LOAD_STORE_UNSIGNED_BYTE	= (1UL << 22),		/* Default is word access */

	/* This fiels gives info about either loading the data or storing it */
	LOAD_INSN			= (1UL << 20),		/* Default is store insn */

	/* This field gives info about either this insn should update the CSPR or not */
	S_BIT_HIGH			= (1UL << 20),

	/*
	 * This field gives info about the second operand involved in the data
	 * processing instructions
	 */
	USE_IMM_OPERAND			= (1UL << 25),		/* Default is register operand */

	/* OFFSET for load and store insns */
	REG_OFFSET			= (1UL << 25),		/* Default is immediate offset */

	/*
	 * Write the modified address to base register for load and store
	 * instructions
	 */
	WRITE_BASE_REG			= (1UL << 21),
};

enum arm_addressing_mode {
	IMM_OFFSET_ADD		= (0x0CUL << 21),
	IMM_OFFSET_SUB		= (0x08UL << 21),
	REG_OFFSET_ADD		= (0x1CUL << 21),
	REG_OFFSET_SUB		= (0x18UL << 21),
	/* Addressing modes for multiple load and store insntructions */
	DECREMENT_BEFORE	= (0x4UL << 22),
};

#define OPCODE(opc)	(opc << 21)

/*
 * ARM instruction encoding.
 */
static uint32_t arm_encode_table[] = {
	[INSN_ADD_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0x4),
	[INSN_ADC_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0x5),
	[INSN_ADC_REG_REG]		= AL | DATA_PROCESSING | OPCODE(0x5),
	[INSN_ADDS_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | S_BIT_HIGH | OPCODE(0x4),
	[INSN_ADD_REG_REG]		= AL | DATA_PROCESSING | OPCODE(0x4),
	[INSN_ADDS_REG_REG]		= AL | DATA_PROCESSING | S_BIT_HIGH | OPCODE(0x4),
	[INSN_MOV_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0xD),
	[INSN_MOV_REG_REG]		= AL | DATA_PROCESSING | OPCODE(0xD),
	[INSN_MVN_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0xF),
	[INSN_LDR_REG_MEMLOCAL]		= AL | LOAD_STORE | LOAD_INSN,
	[INSN_LDR_REG_POOL_IMM]		= AL | LOAD_STORE | LOAD_INSN,
	[INSN_STR_MEMLOCAL_REG]		= AL | LOAD_STORE,
	[INSN_SUB_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0x2),
	[INSN_SBC_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0x6),
	[INSN_SBC_REG_REG]		= AL | DATA_PROCESSING | OPCODE(0x6),
	[INSN_SUBS_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | S_BIT_HIGH | OPCODE(0x2),
	[INSN_SUB_REG_REG]		= AL | DATA_PROCESSING | OPCODE(0x2),
	[INSN_SUBS_REG_REG]		= AL | DATA_PROCESSING | S_BIT_HIGH | OPCODE(0x2),
	[INSN_UNCOND_BRANCH]		= AL | BRANCH | USE_IMM_OPERAND,
	[INSN_PHI]			= INVALID_INSN,
};

static enum machine_reg arg_regs[] = {
	[0]	= MACH_REG_R0,
	[1]	= MACH_REG_R1,
	[2]	= MACH_REG_R2,
	[3]	= MACH_REG_R3,
};

static inline void emit(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);
	assert(!err);
}

static void emit32(struct buffer *buf, uint32_t encoded_insn)
{
	union {
		uint32_t encoded_insn;
		unsigned char b[4];
	} insn_buf;

	insn_buf.encoded_insn = encoded_insn;
	emit(buf, insn_buf.b[0]);
	emit(buf, insn_buf.b[1]);
	emit(buf, insn_buf.b[2]);
	emit(buf, insn_buf.b[3]);
}

uint32_t encode_base_reg_load(struct insn *insn)
{
	switch (insn->src.type) {
	case OPERAND_MEMLOCAL:
		return 0UL | ((arm_encode_reg(MACH_REG_FP) & 0xF) << 16);
	default:
		return 0UL;
	}
}

uint32_t encode_base_reg_store(struct insn *insn)
{
	switch (insn->dest.type) {
	case OPERAND_MEMLOCAL:
		return 0UL | ((arm_encode_reg(MACH_REG_FP) & 0xF) << 16);
	default:
		return 0UL;
	}
}

uint32_t encode_imm_offset_load(struct insn *insn)
{
	long offset;
	switch (insn->src.type) {
	case OPERAND_MEMLOCAL:
		offset = slot_offset(insn->src.slot);
		if (offset < 0) {
			offset = offset * (-1);
			return 0UL | IMM_OFFSET_SUB | (offset & 0xFFF);
		} else
			return 0UL | IMM_OFFSET_ADD | (offset & 0xFFF);
	default:
		return 0UL;
	}
}

uint32_t encode_imm_offset_store(struct insn *insn)
{
	long offset;
	switch (insn->dest.type) {
	case OPERAND_MEMLOCAL:
		offset = slot_offset(insn->dest.slot);
		if (offset < 0) {
			offset = offset * (-1);
			return 0UL | IMM_OFFSET_SUB | (offset & 0xFFF);
		} else
			return 0UL | IMM_OFFSET_ADD | (offset & 0xFFF);
	default:
		return 0UL;
	}
}

void encode_stm(struct buffer *buffer, uint16_t register_list)
{
	uint32_t encoded_insn;
	encoded_insn = AL | MULTIPLE_LOAD_STORE | WRITE_BASE_REG | DECREMENT_BEFORE |
			((arm_encode_reg(MACH_REG_SP) & 0xF) << 16) | register_list;

	emit32(buffer, encoded_insn);
}

void encode_setup_fp(struct buffer *buffer, unsigned long offset)
{
	uint32_t encoded_insn;

	encoded_insn = arm_encode_table[INSN_ADD_REG_IMM];
	encoded_insn = encoded_insn | ((arm_encode_reg(MACH_REG_SP) & 0xF) << 16) |
		((arm_encode_reg(MACH_REG_FP) & 0xF) << 12) | (offset & 0xFF);

	emit32(buffer, encoded_insn);
}

void encode_sub_sp(struct buffer *buffer, unsigned long frame_size)
{
	uint32_t encoded_insn;

	/*
	 * The max immediate value which can be added or subtracted from
	 * a register is 255 so to make large frame we have to emit
	 * subtract insn more than one time.
	 */
	while (frame_size > MAX_FRAME_SIZE_SUBTRACTED) {
		encoded_insn = arm_encode_table[INSN_SUB_REG_IMM];
		encoded_insn = encoded_insn | arm_encode_reg(MACH_REG_SP) << 12 |
			arm_encode_reg(MACH_REG_SP) << 16 | (0xFC);

		emit32(buffer, encoded_insn);
		frame_size = frame_size - MAX_FRAME_SIZE_SUBTRACTED;
	}

	if (frame_size > 0) {
		encoded_insn = arm_encode_table[INSN_SUB_REG_IMM];
		encoded_insn = encoded_insn | arm_encode_reg(MACH_REG_SP) << 12 |
			arm_encode_reg(MACH_REG_SP) << 16 | (frame_size & 0xFF);

		emit32(buffer, encoded_insn);
	}
}

void encode_store_args(struct buffer *buffer, struct stack_frame *frame)
{
	unsigned long nr_args_to_save;
	unsigned long i;
	long offset = 0;
	uint32_t encoded_insn;

	if (frame->nr_args > 4)
		nr_args_to_save = 4;
	else
		nr_args_to_save = frame->nr_args;

	for (i = 0; i < nr_args_to_save; i++) {
		offset = arg_offset(frame, i);
		offset = offset * (-1);

		encoded_insn = arm_encode_table[INSN_STR_MEMLOCAL_REG];
		encoded_insn = encoded_insn | IMM_OFFSET_SUB | ((arm_encode_reg(MACH_REG_FP) & 0xF) << 16) |
				((arm_encode_reg(arg_regs[i]) & 0xFF) << 12) | (offset & 0xFFF);

		emit32(buffer, encoded_insn);
	}
}

void encode_restore_sp(struct buffer *buffer, unsigned long offset)
{
	uint32_t encoded_insn;
	encoded_insn = arm_encode_table[INSN_ADD_REG_IMM];

	encoded_insn = encoded_insn | ((arm_encode_reg(MACH_REG_FP) & 0xF) << 16) |
		((arm_encode_reg(MACH_REG_SP) & 0xF) << 12) | (offset & 0xFF);

	emit32(buffer, encoded_insn);
}

void encode_ldm(struct buffer *buffer, uint16_t register_list)
{
	uint32_t encoded_insn;
	encoded_insn = AL | MULTIPLE_LOAD_STORE | LOAD_INSN | DECREMENT_BEFORE |
			((arm_encode_reg(MACH_REG_SP) & 0xF) << 16) | register_list;

	emit32(buffer, encoded_insn);
}

/*
 * We have to pass the address of cu so that we can call magic_trampoline
 * But addr of cu is of 32 bit and we can use only 8-bit imm so
 * we need to have addr of cu in our constant pool which will be emitted
 * at the starting.
 */
void encode_setup_trampoline(struct buffer *buffer, uint32_t cu_addr, uint32_t target_addr)
{
	uint32_t encoded_insn;
	/*
	 * Branch to the call instruction directly
	 * This branch is added becaouse just after this insn addresses
	 * of cu and jit_magic_trampoline are emitted
	 * which are not instructions
	 */
	encoded_insn = arm_encode_table[INSN_UNCOND_BRANCH];
	encoded_insn = encoded_insn | ((0x000004) >> 2 & 0xFFFFFF);

	emit32(buffer, encoded_insn);

	/* Emit the address of cu */
	emit32(buffer, cu_addr);

	/* Emit the address of magic_tampoline */
	emit32(buffer, target_addr);

	/* Load the addr of cu in R0 */
	encoded_insn = arm_encode_table[INSN_LDR_REG_MEMLOCAL];
	encoded_insn = encoded_insn | IMM_OFFSET_SUB | ((arm_encode_reg(MACH_REG_R0) & 0xF) << 12) |
			((arm_encode_reg(MACH_REG_PC) & 0xF) << 16) | (0x010);

	emit32(buffer, encoded_insn);

	/* Call jit_magic_trampoline. First store the value of PC in LR
	   and then Load the address of magic_trampoline form constant pool */
	encoded_insn = arm_encode_table[INSN_SUB_REG_IMM];
	encoded_insn = encoded_insn | ((arm_encode_reg(MACH_REG_LR) & 0xF) << 12) |
			((arm_encode_reg(MACH_REG_PC) & 0xF) << 16) | (0x000);

	emit32(buffer, encoded_insn);

	encoded_insn = arm_encode_table[INSN_LDR_REG_MEMLOCAL];
	encoded_insn = encoded_insn | IMM_OFFSET_SUB | ((arm_encode_reg(MACH_REG_PC) & 0xF) << 12) |
			((arm_encode_reg(MACH_REG_PC) & 0xF) << 16) | (0x014);

	emit32(buffer, encoded_insn);
}

void encode_emit_branch_link(struct buffer *buffer)
{
	uint32_t encoded_insn = arm_encode_table[INSN_SUB_REG_IMM];
	encoded_insn = encoded_insn | ((arm_encode_reg(MACH_REG_LR) & 0xF) << 12) |
			((arm_encode_reg(MACH_REG_PC) & 0xF) << 16) | (0x000);

	emit32(buffer, encoded_insn);

	encoded_insn = arm_encode_table[INSN_MOV_REG_REG];
	encoded_insn = encoded_insn | ((arm_encode_reg(MACH_REG_PC) & 0xF) << 12) |
			(arm_encode_reg(MACH_REG_R0) & 0xF);

	emit32(buffer, encoded_insn);
}

void insn_encode(struct insn *insn, struct buffer *buffer, struct basic_block *bb)
{
	uint32_t encoded_insn = arm_encode_table[insn->type];

	if (encoded_insn & LOAD_STORE) {
		if (encoded_insn & LOAD_INSN) {
			encoded_insn = encoded_insn | ((arm_encode_reg(mach_reg(&insn->dest.reg)) & 0xF) << 12);
			encoded_insn = encoded_insn | encode_base_reg_load(insn);
			if (!(encoded_insn & REG_OFFSET))
				encoded_insn = encoded_insn | encode_imm_offset_load(insn);
		} else {
			encoded_insn = encoded_insn | ((arm_encode_reg(mach_reg(&insn->src.reg)) & 0xF) << 12);
			encoded_insn = encoded_insn | encode_base_reg_store(insn);
			if (!(encoded_insn & REG_OFFSET))
				encoded_insn = encoded_insn | encode_imm_offset_store(insn);
		}
	} else if (encoded_insn & BRANCH) {
		encoded_insn = encoded_insn | ((emit_branch(insn, bb) >> 2) & 0xFFFFFF);
	} else {
		encoded_insn = encoded_insn | ((arm_encode_reg(mach_reg(&insn->dest.reg)) & 0xF) << 12);
		if (encoded_insn & USE_IMM_OPERAND)
			encoded_insn = encoded_insn | ((insn->src.imm) & 0xFF);
		else
			encoded_insn = encoded_insn | (arm_encode_reg(mach_reg(&insn->src.reg)) & 0xF);

		if (!(insn->type == INSN_MOV_REG_IMM || insn->type == INSN_MOV_REG_REG || insn->type == INSN_MVN_REG_IMM))
			encoded_insn = encoded_insn | ((arm_encode_reg(mach_reg(&insn->dest.reg)) & 0xF) << 16);
	}
	emit32(buffer, encoded_insn);
}
