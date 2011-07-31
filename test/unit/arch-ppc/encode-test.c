#include "arch/encode.h"

#include "arch/instruction.h"

#include "jit/use-position.h"

#include "lib/buffer.h"

#include "libharness.h"

#include <stdint.h>

#define DEFINE_REG(m, n)	struct live_interval n = { .reg = m }

static DEFINE_REG(MACH_REG_R0, reg_r0);
static DEFINE_REG(MACH_REG_R1, reg_r1);
static DEFINE_REG(MACH_REG_R2, reg_r2);
static DEFINE_REG(MACH_REG_R3, reg_r3);
static DEFINE_REG(MACH_REG_R4, reg_r4);
static DEFINE_REG(MACH_REG_R5, reg_r5);
static DEFINE_REG(MACH_REG_R6, reg_r6);
static DEFINE_REG(MACH_REG_R7, reg_r7);

static struct buffer		*buffer;

static void setup(void)
{
	buffer		= alloc_buffer();
}

static void teardown(void)
{
	free_buffer(buffer);
}

void test_encoding_call_reg_high(void)
{
	uint32_t encoding = 0x3c60dead;
	struct insn insn = { };

	setup();

	/* lis     r3,0xdead */
	insn.type			= INSN_LIS;
	insn.src.type			= OPERAND_IMM;
	insn.src.imm			= 0xdead;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r3;

	insn_encode(&insn, buffer, NULL);

	buffer_flip(buffer);

	assert_int_equals(encoding, buffer_read_be32(buffer));

	teardown();
}
