#include "arch/encode.h"

#include "arch/instruction.h"
#include "arch/stack-frame.h"

#include "lib/buffer.h"

#include "vm/die.h"

#include <inttypes.h>
#include <stdint.h>

#define INVALID_INSN		0

static uint8_t arm_register_numbers[] = {
	[MACH_REG_R0] = 0x00,
	[MACH_REG_R1] = 0x01,
	[MACH_REG_R2] = 0x02,
	[MACH_REG_R3] = 0x03,
	[MACH_REG_R4] = 0x04,
	[MACH_REG_R5] = 0x05,
	[MACH_REG_R6] = 0x06,
	[MACH_REG_R7] = 0x07,
	[MACH_REG_R8] = 0x08,
	[MACH_REG_R9] = 0x09,
	[MACH_REG_R10] = 0x0A,
	[MACH_REG_FP] = 0x0B,
	[MACH_REG_IP] = 0x0C,
	[MACH_REG_SP] = 0x0D,
	[MACH_REG_LR] = 0x0E,
	[MACH_REG_PC] = 0x0F,
};

 /* arm_encode_reg : Encodes the register for arm instructions */

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

	/* This field give information about acccessing */
	LOAD_STORE_UNSIGNED_BYTE	= (1UL << 22), /* Default is word access */

	/* This fiels gives info about either loading the data or storing it */
	LOAD_INSN			= (1UL << 20), /* Default is store insn */

	/* This field gives info about either this insn should update the CSPR or not */
	S_BIT_HIGH			= (1UL << 20),

	/*
	 * This field gives info about the second operand involved
	 * in the data processing instructions
	 */
	USE_IMM_OPERAND			= (1UL << 25), /* Default is register operand */
	/* OFFSET for load and store insns */
	REG_OFFSET			= (1UL << 25), /* Default is immediate offset */
};

enum arm_addressing_mode {
	IMM_OFFSET_ADD	= (0x0CUL << 21),
	IMM_OFFSET_SUB	= (0x08UL << 21),
	REG_OFFSET_ADD  = (0x1CUL << 21),
	REG_OFFSET_SUB	= (0x18UL << 21),
};

#define OPCODE(opc)	(opc << 21)

/* This is the list of ARM instruction field encodings */

static uint32_t arm_encode_insn[] = {
	[INSN_MOV_REG_IMM]		= AL | DATA_PROCESSING | USE_IMM_OPERAND | OPCODE(0xD),
	[INSN_LOAD_REG_MEMLOCAL]	= AL | LOAD_STORE | LOAD_INSN,
	[INSN_LOAD_REG_POOL_IMM]	= AL | LOAD_STORE | LOAD_INSN,
	[INSN_PHI]			= INVALID_INSN,
};

static inline void emit(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);
	assert(!err);
}

static void emit_encoded_insn(struct buffer *buf, uint32_t encoded_insn)
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

uint32_t encode_base_reg(struct insn *insn)
{
	switch (insn->src.type || insn->dest.type) {
	case OPERAND_MEMLOCAL:
		return 0UL | ((arm_encode_reg(MACH_REG_FP) & 0xF) << 16);
	default:
		return 0UL;
	}
}

uint32_t encode_imm_offset(struct insn *insn)
{
	long offset;
	switch (insn->src.type || insn->dest.type) {
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

void insn_encode(struct insn *insn, struct buffer *buffer, struct basic_block *bb)
{
	uint32_t encoded_insn = arm_encode_insn[insn->type];
	encoded_insn = encoded_insn | ((arm_encode_reg(mach_reg(&insn->dest.reg)) & 0xF) << 12);

	if (encoded_insn & USE_IMM_OPERAND)
		encoded_insn = encoded_insn | ((insn->src.imm) & 0xFF);

	if (encoded_insn & LOAD_STORE) {
		encoded_insn = encoded_insn | encode_base_reg(insn);
		if (!(encoded_insn & REG_OFFSET))
			encoded_insn = encoded_insn | encode_imm_offset(insn);
	}
	emit_encoded_insn(buffer, encoded_insn);
}
