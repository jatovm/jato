#include "arch/encode.h"

#include "arch/instruction.h"

#include "jit/use-position.h"

#include "lib/buffer.h"

#include "libharness.h"

#include <stdint.h>

#define DEFINE_REG(m, n)	struct live_interval n = { .reg = m }

static DEFINE_REG(MACH_REG_xAX, reg_eax);
static DEFINE_REG(MACH_REG_xBX, reg_ebx);
static DEFINE_REG(MACH_REG_xSI, reg_esi);
static DEFINE_REG(MACH_REG_xDI, reg_edi);
static DEFINE_REG(MACH_REG_xSP, reg_esp);
static DEFINE_REG(MACH_REG_XMM7, reg_xmm7);

#ifdef CONFIG_X86_64
static DEFINE_REG(MACH_REG_RAX, reg_rax);
static DEFINE_REG(MACH_REG_RBX, reg_rbx);
static DEFINE_REG(MACH_REG_RBP, reg_rbp);
static DEFINE_REG(MACH_REG_RSP, reg_rsp);
static DEFINE_REG(MACH_REG_R12, reg_r12);
static DEFINE_REG(MACH_REG_R13, reg_r13);
static DEFINE_REG(MACH_REG_R14, reg_r14);
static DEFINE_REG(MACH_REG_R15, reg_r15);
#endif

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

void test_encoding_call_reg(void)
{
	uint8_t encoding[] = { 0xff, 0x13 };
	struct insn insn = { };

	setup();

	/* mov    *(%ebx) */
	insn.type			= INSN_CALL_REG;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_call_reg_high(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x41, 0xff, 0x17 };
	struct insn insn = { };

	setup();

	/* mov    *(%r15) */
	insn.type			= INSN_CALL_REG;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r15;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_call_reg_rbp(void)
{
#ifdef CONFIG_X86_64
	/* rbp is special and needs a forced displacement */
	uint8_t encoding[] = { 0xff, 0x55, 0x00 };
	struct insn insn = { };

	setup();

	/* mov    *(%rbp) */
	insn.type			= INSN_CALL_REG;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_rbp;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_call_reg_r12(void)
{
#ifdef CONFIG_X86_64
	/* rbp is special and needs a forced SIB */
	uint8_t encoding[] = { 0x41, 0xff, 0x14, 0x24 };
	struct insn insn = { };

	setup();

	/* mov    *(%rbp) */
	insn.type			= INSN_CALL_REG;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r12;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_call_reg_r13(void)
{
#ifdef CONFIG_X86_64
	/* r13 is special and needs a forced displacement */
	uint8_t encoding[] = { 0x41, 0xff, 0x55, 0x00 };
	struct insn insn = { };

	setup();

	/* mov    *(%r13) */
	insn.type			= INSN_CALL_REG;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r13;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_imm_reg(void)
{
	uint8_t encoding[] = { 0x81, 0xd3, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    $0x12345678,%ebx */
	insn.type			= INSN_ADC_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_imm8_reg(void)
{
	uint8_t encoding[] = { 0xc1, 0xfb, 0x1f };
	struct insn insn = { };

	setup();

	/* sar    $0x1f,%ebx */
	insn.type			= INSN_SAR_IMM_REG;
	insn.src.imm			= 0x1f;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_imm_reg_r12(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x49, 0x81, 0xd4, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    $0x12345678,%ebx */
	insn.type			= INSN_ADC_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r12;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_imm_reg_rsp(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x48, 0x81, 0xc4, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* add    $0x12345678,%rsp */
	insn.type			= INSN_ADD_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_rsp;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_imm_reg_esp(void)
{
	uint8_t encoding[] = { 0x81, 0xd4, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    $0x12345678,%esp */
	insn.type			= INSN_ADC_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.reg.interval		= &reg_esp;

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

void test_encoding_jmp_membase(void)
{
	uint8_t encoding[] = { 0xff, 0xa0, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* jmp    *0xdeadbeef(%eax) */
	insn.type			= INSN_JMP_MEMBASE;
	insn.dest.base_reg.interval	= &reg_eax;
	insn.dest.disp			= 0x12345678;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_jmp_memindex(void)
{
	uint8_t encoding[] = { 0xff, 0x24, 0xbe };
	struct insn insn = { };

	setup();

	/* jmp    *0xdeadbeef(%eax) */
	insn.type			= INSN_JMP_MEMINDEX;
	insn.dest.base_reg.interval	= &reg_esi;
	insn.dest.index_reg.interval	= &reg_edi;
	insn.dest.shift			= 2;

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
	insn.type			= INSN_MOVSS_XMM_MEMBASE;
	insn.src.reg.interval		= &reg_xmm7;
	insn.dest.base_reg.interval	= &reg_esp;
	insn.dest.disp			= 0x0;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_rex_imm_reg(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x48, 0x81, 0xd3, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    $0x12345678,%ebx */
	insn.type			= INSN_ADC_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_rbx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_rex_imm_reg_high(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x49, 0x81, 0xd7, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* mov    $0x12345678,%r15 */
	insn.type			= INSN_ADC_IMM_REG;
	insn.src.imm			= 0x12345678;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r15;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_rex_reg_reg_low_low(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x48, 0x01, 0xc3 };
	struct insn insn = { };

	setup();

	/* mov    %rax,%rbx */
	insn.type			= INSN_ADD_REG_REG;
	insn.src.type			= OPERAND_REG;
	insn.src.reg.interval		= &reg_rax;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_rbx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_rex_reg_reg_low_high(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x49, 0x01, 0xc7 };
	struct insn insn = { };

	setup();

	/* mov    %rax,%r15 */
	insn.type			= INSN_ADD_REG_REG;
	insn.src.type			= OPERAND_REG;
	insn.src.reg.interval		= &reg_rax;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r15;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_rex_reg_reg_high_low(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x4c, 0x01, 0xf8 };
	struct insn insn = { };

	setup();

	/* mov    %r15,%rax */
	insn.type			= INSN_ADD_REG_REG;
	insn.src.type			= OPERAND_REG;
	insn.src.reg.interval		= &reg_r15;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_rax;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_rex_reg_reg_high_high(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x4d, 0x01, 0xf7 };
	struct insn insn = { };

	setup();

	/* mov    %r14,%r15 */
	insn.type			= INSN_ADD_REG_REG;
	insn.src.type			= OPERAND_REG;
	insn.src.reg.interval		= &reg_r14;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_r15;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_reg(void)
{
	uint8_t encoding[] = { 0xf7, 0xdb };
	struct insn insn = { };

	setup();

	/* neg	%ebx */
	insn.type			= INSN_NEG_REG;
	insn.dest.reg.interval		= &reg_ebx;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_high(void)
{
#ifdef CONFIG_X86_64
	uint8_t encoding[] = { 0x41, 0x57 };
	struct insn insn = { };

	setup();

	/* push	%r15 */
	insn.type			= INSN_PUSH_REG;
	insn.src.type			= OPERAND_REG;
	insn.src.reg.interval		= &reg_r15;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
#endif
}

void test_encoding_memdisp_reg(void)
{
	uint8_t encoding[] = { 0xf2, 0x0f, 0x10, 0x3d, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* movsd  0x12345678,%xmm7 */
	insn.type			= INSN_MOVSD_MEMDISP_XMM;
	insn.src.imm			= 0x12345678;
	insn.dest.type			= OPERAND_REG;
	insn.dest.reg.interval		= &reg_xmm7;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_reg_memdisp(void)
{
	uint8_t encoding[] = { 0xf2, 0x0f, 0x11, 0x3d, 0x78, 0x56, 0x34, 0x12 };
	struct insn insn = { };

	setup();

	/* movsd  %xmm7,0x12345678 */
	insn.type			= INSN_MOVSD_XMM_MEMDISP;
	insn.src.type			= OPERAND_REG;
	insn.src.reg.interval		= &reg_xmm7;
	insn.dest.imm			= 0x12345678;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_memlocal(void)
{
#ifdef CONFIG_32_BIT
	uint8_t encoding[] = { 0xff, 0xb5, 0x14, 0x00, 0x00, 0x00 };
#else
	uint8_t encoding[] = { 0xff, 0xb5, 0x10, 0x00, 0x00, 0x00 };
#endif
	struct stack_frame stack_frame;
	struct stack_slot local_slot;
	struct insn insn = { };

	stack_frame	= (struct stack_frame) {
		.nr_args		= 1,
	};
	local_slot	= (struct stack_slot) {
		.parent			= &stack_frame,
		.index			= 0x0,
	};

	setup();

	/* pushl  0x12345678(%ebp) */
	insn.type			= INSN_PUSH_MEMLOCAL;
	insn.src.slot			= &local_slot;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_fstp_memlocal(void)
{
#ifdef CONFIG_32_BIT
	uint8_t encoding[] = { 0xd9, 0x9d, 0x14, 0x00, 0x00, 0x00 };
#else
	uint8_t encoding[] = { 0xd9, 0x9d, 0x10, 0x00, 0x00, 0x00 };
#endif
	struct stack_frame stack_frame;
	struct stack_slot local_slot;
	struct insn insn = { };

	stack_frame	= (struct stack_frame) {
		.nr_args		= 1,
	};
	local_slot	= (struct stack_slot) {
		.parent			= &stack_frame,
		.index			= 0x0,
	};

	setup();

	/* fstps	$0x14(%ebp)|$0x10(%rbp) */
	insn.type			= INSN_FSTP_MEMLOCAL;
	insn.src.slot			= &local_slot;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}

void test_encoding_fstp_64_memlocal(void)
{
#ifdef CONFIG_32_BIT
	uint8_t encoding[] = { 0xdd, 0x9d, 0x14, 0x00, 0x00, 0x00 };
#else
	uint8_t encoding[] = { 0xdd, 0x9d, 0x10, 0x00, 0x00, 0x00 };
#endif
	struct stack_frame stack_frame;
	struct stack_slot local_slot;
	struct insn insn = { };

	stack_frame	= (struct stack_frame) {
		.nr_args		= 1,
	};
	local_slot	= (struct stack_slot) {
		.parent			= &stack_frame,
		.index			= 0x0,
	};

	setup();

	/* fstpl	$0x12345678(%ebp) */
	insn.type			= INSN_FSTP_64_MEMLOCAL;
	insn.src.slot			= &local_slot;

	insn_encode(&insn, buffer, NULL);

	assert_int_equals(ARRAY_SIZE(encoding), buffer_offset(buffer));
	assert_mem_equals(encoding, buffer_ptr(buffer), ARRAY_SIZE(encoding));

	teardown();
}
