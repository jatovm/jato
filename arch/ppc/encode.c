#include "arch/encode.h"

#include "arch/instruction.h"

#include "lib/buffer.h"
#include "vm/die.h"

static uint8_t ppc_register_numbers[] = {
	[MACH_REG_R0]		= 0,
	[MACH_REG_R1]		= 1,
	[MACH_REG_R2]		= 2,
	[MACH_REG_R3]		= 3,
	[MACH_REG_R4]		= 4,
	[MACH_REG_R5]		= 5,
	[MACH_REG_R6]		= 6,
	[MACH_REG_R7]		= 7,
	[MACH_REG_R8]		= 8,
	[MACH_REG_R9]		= 9,
	[MACH_REG_R10]		= 10,
	[MACH_REG_R11]		= 11,
	[MACH_REG_R12]		= 12,
	[MACH_REG_R13]		= 13,
	[MACH_REG_R14]		= 14,
	[MACH_REG_R15]		= 15,
	[MACH_REG_R16]		= 16,
	[MACH_REG_R17]		= 17,
	[MACH_REG_R18]		= 18,
	[MACH_REG_R19]		= 19,
	[MACH_REG_R20]		= 20,
	[MACH_REG_R21]		= 21,
	[MACH_REG_R22]		= 22,
	[MACH_REG_R23]		= 23,
	[MACH_REG_R24]		= 24,
	[MACH_REG_R25]		= 25,
	[MACH_REG_R26]		= 26,
	[MACH_REG_R27]		= 27,
	[MACH_REG_R28]		= 28,
	[MACH_REG_R29]		= 29,
	[MACH_REG_R30]		= 30,
	[MACH_REG_R31]		= 31,
};

static uint8_t ppc_encode_reg(enum machine_reg reg)
{
	if (reg == MACH_REG_UNASSIGNED)
		die("unassigned register during code emission");

	if (reg < 0 || reg >= ARRAY_SIZE(ppc_register_numbers))
		 die("unknown register %d", reg);

	return ppc_register_numbers[reg];
}

static uint8_t encode_reg(struct operand *operand)
{
	enum machine_reg reg = mach_reg(&operand->reg);

	return ppc_encode_reg(reg);
}

static void emit(struct buffer *b, unsigned long insn)
{
	buffer_write_be32(b, insn);
}

void insn_encode(struct insn *insn, struct buffer *buffer, struct basic_block *bb)
{
	uint32_t encoding;

	switch (insn->type) {
	case INSN_LIS:
		encoding = lis(encode_reg(&insn->dest), insn->src.imm);
		break;
	default:
		die("Unknown instruction type: %d\n", insn->type);
		break;
	}

	emit(buffer, encoding);
}
