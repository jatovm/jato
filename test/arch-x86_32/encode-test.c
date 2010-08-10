#include "arch/encode.h"

#include "arch/instruction.h"

#include "jit/use-position.h"

#include "lib/buffer.h"

#include "libharness.h"

#include <stdint.h>

#define DEFINE_REG(m, n)	struct live_interval n = { .reg = m }

static DEFINE_REG(MACH_REG_EAX, reg_eax);
static DEFINE_REG(MACH_REG_EBX, reg_ebx);
static DEFINE_REG(MACH_REG_EDI, reg_edi);
static DEFINE_REG(MACH_REG_ESP, reg_esp);
static DEFINE_REG(MACH_REG_XMM7, reg_xmm7);

static struct buffer		*buffer;

static void setup(void)
{
	buffer		= alloc_buffer();
}

static void teardown(void)
{
	free_buffer(buffer);
}

void test_encoding_one_byte(void)
{
	uint8_t encoding[] = { 0xc3 };
	struct insn insn = { };

	setup();

	/* mov    (%eax),%ebx */
	insn.type			= INSN_RET;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_imm_reg(void)
{
	uint8_t encoding[] = { 0x81, 0xd3, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    %eax,%ebx */
	insn.type			= INSN_ADC_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_reg(void)
{
	uint8_t encoding[] = { 0x89, 0xc3 };
	struct insn insn = { };

	setup();

	/* mov    %eax,%ebx */
	insn.type			= INSN_MOV_REG_REG;
	insn.src.reg.interval		= &reg_eax;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_mem_reg(void)
{
	uint8_t encoding[] = { 0x8b, 0x18 };
	struct insn insn = { };

	setup();

	/* mov    (%eax),%ebx */
	insn.type			= INSN_MOV_MEMBASE_REG;
	insn.src.base_reg.interval	= &reg_eax;
	insn.src.disp			= 0x00000000;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_membase_8_reg(void)
{
	uint8_t encoding[] = { 0x8b, 0x58, 0x14 };
	struct insn insn = { };

	setup();

	/* mov    0x14(%eax),%ebx */
	insn.type			= INSN_MOV_MEMBASE_REG;
	insn.src.base_reg.interval	= &reg_eax;
	insn.src.disp			= 0x00000014;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_membase_32_reg(void)
{
	uint8_t encoding[] = { 0x8b, 0x98, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    0x12345678(%eax),%ebx */
	insn.type			= INSN_MOV_MEMBASE_REG;
	insn.src.base_reg.interval	= &reg_eax;
	insn.src.disp			= 0x12345678;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_mem_reg_sib(void)
{
	uint8_t encoding[] = { 0x8b, 0x3c, 0x24 };
	struct insn insn = { };

	setup();

	/* mov    (%esp),%edi */
	insn.type			= INSN_MOV_MEMBASE_REG;
	insn.src.base_reg.interval	= &reg_esp;
	insn.src.disp			= 0x00000000;
	insn.dest.reg.interval		= &reg_edi;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_membase_8_reg_sib(void)
{
	uint8_t encoding[] = { 0x8b, 0x7c, 0x24, 0x14 };
	struct insn insn = { };

	setup();

	/* mov    0x14(%esp),%edi */
	insn.type			= INSN_MOV_MEMBASE_REG;
	insn.src.base_reg.interval	= &reg_esp;
	insn.src.disp			= 0x00000014;
	insn.dest.reg.interval		= &reg_edi;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_membase_32_reg_sib(void)
{
	uint8_t encoding[] = { 0x8b, 0xbc, 0x24, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    0x12345678(%esp),%edi */
	insn.type			= INSN_MOV_MEMBASE_REG;
	insn.src.base_reg.interval	= &reg_esp;
	insn.src.disp			= 0x12345678;
	insn.dest.reg.interval		= &reg_edi;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_mem(void)
{
	uint8_t encoding[] = { 0x89, 0x03 };
	struct insn insn = { };

	setup();

	/* mov    %eax,(%ebx) */
	insn.type			= INSN_MOV_REG_MEMBASE;
	insn.src.reg.interval		= &reg_eax;
	insn.dest.base_reg.interval	= &reg_ebx;
	insn.dest.disp			= 0x00000000;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_membase_8(void)
{
	uint8_t encoding[] = { 0x89, 0x43, 0x14 };
	struct insn insn = { };

	setup();

	/* mov    %eax,0x14(%ebx) */
	insn.type			= INSN_MOV_REG_MEMBASE;
	insn.src.reg.interval		= &reg_eax;
	insn.dest.base_reg.interval	= &reg_ebx;
	insn.dest.disp			= 0x00000014;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_membase_32(void)
{
	uint8_t encoding[] = { 0x89, 0x83, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    %eax,0x12345678(%ebx) */
	insn.type			= INSN_MOV_REG_MEMBASE;
	insn.src.reg.interval		= &reg_eax;
	insn.dest.base_reg.interval	= &reg_ebx;
	insn.dest.disp			= 0x12345678;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_membase_xmm(void)
{
	uint8_t encoding[] = { 0xf3, 0x0f, 0x11, 0x3c, 0x24 };
	struct insn insn = { };

	setup();

	/* movss  %xmm7,(%esp) */
	insn.type			= INSN_MOV_XMM_MEMBASE;
	insn.src.reg.interval		= &reg_xmm7;
	insn.dest.base_reg.interval	= &reg_esp;
	insn.dest.disp			= 0x0;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}
