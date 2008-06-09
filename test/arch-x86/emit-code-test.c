/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/compilation-unit.h>
#include <jit/basic-block.h>
#include <jit/statement.h>

#include <vm/buffer.h>
#include <vm/list.h>
#include <vm/system.h>
#include <vm/vm.h>

#include <arch/emit-code.h>
#include <arch/instruction.h>

#include <test/vars.h>
#include <libharness.h>

static struct methodblock method;

DECLARE_STATIC_REG(VAR_EAX, REG_EAX);
DECLARE_STATIC_REG(VAR_EBX, REG_EBX);
DECLARE_STATIC_REG(VAR_ECX, REG_ECX);
DECLARE_STATIC_REG(VAR_EDX, REG_EDX);

DECLARE_STATIC_REG(VAR_EBP, REG_EBP);
DECLARE_STATIC_REG(VAR_ESP, REG_ESP);

static void assert_emit_insn(unsigned char *expected,
			     unsigned long expected_size,
			     struct insn *insn)
{
	struct basic_block *bb;
	struct buffer *buf;

	bb = alloc_basic_block(NULL, 0, 1);
	bb_add_insn(bb, insn);

	buf = alloc_buffer();
	emit_body(bb, buf);
	assert_int_equals(expected_size, buffer_offset(buf));
	assert_mem_equals(expected, buffer_ptr(buf), expected_size);
	
	free_basic_block(bb);
	free_buffer(buf);
}

static void assert_emit_insn_1(unsigned char expected_opc, struct insn *insn)
{
	unsigned char expected[] = { expected_opc };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

static void assert_emit_insn_2(unsigned char opcode, unsigned char extra,
			       struct insn *insn)
{
	unsigned char expected[] = { opcode, extra };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

static void assert_emit_insn_3(unsigned char opcode, unsigned char modrm,
			       unsigned char extra, struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, extra };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

static void assert_emit_insn_4(unsigned char opcode, unsigned char modrm,
			       unsigned char b1, unsigned char b2, struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, b1, b2 };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

static void assert_emit_insn_5(unsigned char opcode, unsigned char modrm,
			       unsigned char b1, unsigned char b2,
			       unsigned char b3, struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, b1, b2, b3 };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

static void assert_mem_insn_5(unsigned char expected_1,
			      unsigned char expected_2,
			      unsigned char expected_3,
			      unsigned char expected_4,
			      unsigned char expected_5,
			      const char *actual)
{
	unsigned char expected[] = { expected_1, expected_2, expected_3, expected_4, expected_5 };

	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

static void assert_emit_insn_6(unsigned char opcode, unsigned char modrm,
			       unsigned char b1, unsigned char b2,
			       unsigned char b3, unsigned char b4,
			       struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, b1, b2, b3, b4 };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

static void assert_mem_insn_6(unsigned char expected_1,
			      unsigned char expected_2,
			      unsigned char expected_3,
			      unsigned char expected_4,
			      unsigned char expected_5,
			      unsigned char expected_6,
			      const char *actual)
{
	unsigned char expected[] = { expected_1, expected_2, expected_3, expected_4, expected_5, expected_6 };

	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

static void assert_emit_insn_7(unsigned char opcode, unsigned char modrm,
			       unsigned char b1, unsigned char b2,
			       unsigned char b3, unsigned char b4,
			       unsigned char b5, struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, b1, b2, b3, b4, b5 };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

void test_emit_prolog_no_locals(void)
{
	unsigned char expected[] = { 0x55, 0x89, 0xe5 };
	struct buffer *buf;

	buf = alloc_buffer();
	emit_prolog(buf, 0);
	assert_mem_equals(expected, buffer_ptr(buf), ARRAY_SIZE(expected));
	free_buffer(buf);
}

void test_emit_prolog_has_locals(void)
{
	unsigned char expected[] = { 0x55, 0x89, 0xe5, 0x81, 0xec, 0x80, 0x00, 0x00, 0x00 };
	struct buffer *buf;

	buf = alloc_buffer();
	emit_prolog(buf, 0x20);
	assert_mem_equals(expected, buffer_ptr(buf), ARRAY_SIZE(expected));
	free_buffer(buf);
}

void test_emit_epilog_no_locals(void)
{
	unsigned char expected[] = { 0x5d, 0xc3 };
	struct buffer *buf;

	buf = alloc_buffer();
	emit_epilog(buf, 0);
	assert_mem_equals(expected, buffer_ptr(buf), ARRAY_SIZE(expected));
	free_buffer(buf);
}

void test_emit_epilog_has_locals(void)
{
	unsigned char expected[] = { 0xc9, 0xc3 };
	struct buffer *buf;

	buf = alloc_buffer();
	emit_epilog(buf, 0x20);
	assert_mem_equals(expected, buffer_ptr(buf), ARRAY_SIZE(expected));
	free_buffer(buf);
}

void test_emit_push_imm(void)
{
	assert_emit_insn_2(0x6a, 0x04, imm_insn(INSN_PUSH_IMM, 0x04));
	assert_emit_insn_5(0x68, 0xef, 0xbe, 0xad, 0xde, imm_insn(INSN_PUSH_IMM, 0xdeadbeef));
}

void test_emit_push_reg(void)
{
	assert_emit_insn_1(0x50, reg_insn(INSN_PUSH_REG, &VAR_EAX));
	assert_emit_insn_1(0x53, reg_insn(INSN_PUSH_REG, &VAR_EBX));
	assert_emit_insn_1(0x51, reg_insn(INSN_PUSH_REG, &VAR_ECX));
	assert_emit_insn_1(0x52, reg_insn(INSN_PUSH_REG, &VAR_EDX));
	assert_emit_insn_1(0x55, reg_insn(INSN_PUSH_REG, &VAR_EBP));
	assert_emit_insn_1(0x54, reg_insn(INSN_PUSH_REG, &VAR_ESP));
}

static void assert_emit_call(long target_offset)
{
	unsigned char expected[5];
	struct basic_block *bb;
	struct buffer *buf;
	void *call_target;
	long disp;
       
	buf = alloc_buffer();
	call_target = buffer_ptr(buf) + target_offset;
	disp = call_target - buffer_ptr(buf) - 5;

	expected[0] = 0xe8;
	expected[1] = (disp & 0x000000ffUL);
	expected[2] = (disp & 0x0000ff00UL) >>  8;
	expected[3] = (disp & 0x00ff0000UL) >> 16;
	expected[4] = (disp & 0xff000000UL) >> 24;

	bb = alloc_basic_block(NULL, 0, 1);
	bb_add_insn(bb, rel_insn(INSN_CALL_REL, (unsigned long) call_target));

	emit_body(bb, buf);
	assert_mem_equals(expected, buffer_ptr(buf), ARRAY_SIZE(expected));

	free_basic_block(bb);
	free_buffer(buf);
}

void test_emit_call_backward(void)
{
	assert_emit_call(-3);
}

void test_emit_call_forward(void)
{
	assert_emit_call(5);
}

void test_emit_indirect_call(void)
{
	assert_emit_insn_2(0xff, 0x10, reg_insn(INSN_CALL_REG, &VAR_EAX));
	assert_emit_insn_2(0xff, 0x13, reg_insn(INSN_CALL_REG, &VAR_EBX));
	assert_emit_insn_2(0xff, 0x11, reg_insn(INSN_CALL_REG, &VAR_ECX));
}

void test_emit_mov_imm(void)
{
	assert_emit_insn_5(0xb8, 0x04, 0x00, 0x00, 0x00, imm_reg_insn(INSN_MOV_IMM_REG, 0x04, &VAR_EAX));
	assert_emit_insn_5(0xb8, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(INSN_MOV_IMM_REG, 0xdeadbeef, &VAR_EAX));
	assert_emit_insn_5(0xbb, 0xbe, 0xba, 0xfe, 0xca, imm_reg_insn(INSN_MOV_IMM_REG, 0xcafebabe, &VAR_EBX));
	assert_emit_insn_5(0xb9, 0xbe, 0xba, 0xfe, 0xca, imm_reg_insn(INSN_MOV_IMM_REG, 0xcafebabe, &VAR_ECX));
	assert_emit_insn_5(0xba, 0xbe, 0xba, 0xfe, 0xca, imm_reg_insn(INSN_MOV_IMM_REG, 0xcafebabe, &VAR_EDX));
}

void test_emit_mov_imm_disp(void)
{
	assert_emit_insn_6(0xc7, 0x00, 0xbe, 0xba, 0xfe, 0xca, imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xcafebabe, &VAR_EAX, 0));
	assert_emit_insn_6(0xc7, 0x03, 0xef, 0xbe, 0xad, 0xde, imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xdeadbeef, &VAR_EBX, 0));
	assert_emit_insn_7(0xc7, 0x40, 0x08, 0xbe, 0xba, 0xfe, 0xca, imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xcafebabe, &VAR_EAX, 0x08));
	assert_emit_insn_7(0xc7, 0x43, 0x0c, 0xbe, 0xba, 0xfe, 0xca, imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xcafebabe, &VAR_EBX, 0x0c));
}

void test_emit_mov_disp_reg(void)
{
	assert_emit_insn_3(0x8b, 0x45, 0x08, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_EBP, 0x08, &VAR_EAX));
	assert_emit_insn_3(0x8b, 0x45, 0x0c, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_EBP, 0x0c, &VAR_EAX));

	assert_emit_insn_3(0x8b, 0x5D, 0x08, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_EBP, 0x08, &VAR_EBX));
	assert_emit_insn_3(0x8b, 0x4D, 0x08, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_EBP, 0x08, &VAR_ECX));
	assert_emit_insn_3(0x8b, 0x55, 0x08, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_EBP, 0x08, &VAR_EDX));
	assert_emit_insn_4(0x8b, 0x44, 0x24, 0x00, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_ESP, 0, &VAR_EAX));

	assert_emit_insn_6(0x8b, 0x80, 0x50, 0x02, 0x00, 0x00, membase_reg_insn(INSN_MOV_MEMBASE_REG, &VAR_EAX, 0x250, &VAR_EAX));
}

void test_emit_mov_memindex_reg(void)
{
	assert_emit_insn_3(0x8b, 0x0c, 0x18, memindex_reg_insn(INSN_MOV_MEMINDEX_REG, &VAR_EAX, &VAR_EBX, 0, &VAR_ECX));
	assert_emit_insn_3(0x8b, 0x14, 0x4b, memindex_reg_insn(INSN_MOV_MEMINDEX_REG, &VAR_EBX, &VAR_ECX, 1, &VAR_EDX));
}

void test_emit_mov_reg_memlocal(void)
{
	struct stack_slot *slot, *wide_slot;
	struct stack_frame *frame;

	frame = alloc_stack_frame(32, 32);
	slot = get_local_slot(frame, 0);
	wide_slot = get_local_slot(frame, 31);

	assert_emit_insn_3(0x89, 0x45, 0x08, reg_memlocal_insn(INSN_MOV_REG_MEMLOCAL, &VAR_EAX, slot));
	assert_emit_insn_6(0x89, 0x9d, 0x84, 0x00, 0x00, 0x00, reg_memlocal_insn(INSN_MOV_REG_MEMLOCAL, &VAR_EBX, wide_slot));

	free_stack_frame(frame);
}

void test_emit_mov_reg_memindex(void)
{
	assert_emit_insn_3(0x89, 0x14, 0x01, reg_memindex_insn(INSN_MOV_REG_MEMINDEX, &VAR_EDX, &VAR_ECX, &VAR_EAX, 0));
	assert_emit_insn_3(0x89, 0x04, 0x4b, reg_memindex_insn(INSN_MOV_REG_MEMINDEX, &VAR_EAX, &VAR_EBX, &VAR_ECX, 1));
}

void test_emit_mov_reg_reg(void)
{
	assert_emit_insn_2(0x89, 0xd0, reg_reg_insn(INSN_MOV_REG_REG, &VAR_EDX, &VAR_EAX));
	assert_emit_insn_2(0x89, 0xd9, reg_reg_insn(INSN_MOV_REG_REG, &VAR_EBX, &VAR_ECX));
}

void test_emit_adc_disp_reg(void)
{
	assert_emit_insn_3(0x13, 0x45, 0x04, membase_reg_insn(INSN_ADC_MEMBASE_REG, &VAR_EBP, 0x04, &VAR_EAX));
}

void test_emit_adc_imm(void)
{
	assert_emit_insn_3(0x83, 0xd0, 0x04, imm_reg_insn(INSN_ADC_IMM_REG, 0x04, &VAR_EAX));
	assert_emit_insn_3(0x83, 0xd3, 0x08, imm_reg_insn(INSN_ADC_IMM_REG, 0x08, &VAR_EBX));
	assert_emit_insn_6(0x81, 0xd3, 0x80, 0x00, 0x00, 0x00, imm_reg_insn(INSN_ADC_IMM_REG, 0x80, &VAR_EBX));
	assert_emit_insn_6(0x81, 0xd0, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(INSN_ADC_IMM_REG, 0xdeadbeef, &VAR_EAX));
}

void test_emit_add_disp_reg(void)
{
	assert_emit_insn_3(0x03, 0x45, 0x04, membase_reg_insn(INSN_ADD_MEMBASE_REG, &VAR_EBP, 0x04, &VAR_EAX));
}

void test_emit_add_imm(void)
{
	assert_emit_insn_3(0x83, 0xc0, 0x04, imm_reg_insn(INSN_ADD_IMM_REG, 0x04, &VAR_EAX));
	assert_emit_insn_3(0x83, 0xc3, 0x08, imm_reg_insn(INSN_ADD_IMM_REG, 0x08, &VAR_EBX));
	assert_emit_insn_6(0x81, 0xc3, 0x80, 0x00, 0x00, 0x00, imm_reg_insn(INSN_ADD_IMM_REG, 0x80, &VAR_EBX));
	assert_emit_insn_6(0x81, 0xc0, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(INSN_ADD_IMM_REG, 0xdeadbeef, &VAR_EAX));
}

void test_emit_sub_membase_reg(void)
{
	assert_emit_insn_3(0x2b, 0x45, 0x04, membase_reg_insn(INSN_SUB_MEMBASE_REG, &VAR_EBP, 0x04, &VAR_EAX));
}

void test_emit_mul_membase_reg(void)
{
	assert_emit_insn_3(0xf7, 0x65, 0x0c, membase_reg_insn(INSN_MUL_MEMBASE_REG, &VAR_EBP, 0x0c, &VAR_EAX));
	assert_emit_insn_6(0xf7, 0xa5, 0xef, 0xbe, 0xad, 0xde, membase_reg_insn(INSN_MUL_MEMBASE_REG, &VAR_EBP, 0xdeadbeef, &VAR_EAX));
}

void test_emit_cltd(void)
{
	assert_emit_insn_1(0x99, reg_reg_insn(INSN_CLTD_REG_REG, &VAR_EAX, &VAR_EDX));
}

void test_emit_div_membase_reg(void)
{
	assert_emit_insn_3(0xf7, 0x7d, 0x0c, membase_reg_insn(INSN_DIV_MEMBASE_REG, &VAR_EBP, 0x0c, &VAR_EAX));
	assert_emit_insn_6(0xf7, 0xbd, 0xef, 0xbe, 0xad, 0xde, membase_reg_insn(INSN_DIV_MEMBASE_REG, &VAR_EBP, 0xdeadbeef, &VAR_EAX));
}

void test_emit_neg_reg(void)
{
	assert_emit_insn_2(0xf7, 0xd8, reg_insn(INSN_NEG_REG, &VAR_EAX));
	assert_emit_insn_2(0xf7, 0xdb, reg_insn(INSN_NEG_REG, &VAR_EBX));
}

void test_emit_shl_reg_reg(void)
{
	assert_emit_insn_2(0xd3, 0xe0, reg_reg_insn(INSN_SHL_REG_REG, &VAR_ECX, &VAR_EAX));
	assert_emit_insn_2(0xd3, 0xe3, reg_reg_insn(INSN_SHL_REG_REG, &VAR_ECX, &VAR_EBX));
}

void test_emit_sar_reg_reg(void)
{
	assert_emit_insn_2(0xd3, 0xf8, reg_reg_insn(INSN_SAR_REG_REG, &VAR_ECX, &VAR_EAX));
	assert_emit_insn_2(0xd3, 0xfb, reg_reg_insn(INSN_SAR_REG_REG, &VAR_ECX, &VAR_EBX));
}

void test_emit_shr_reg_reg(void)
{
	assert_emit_insn_2(0xd3, 0xe8, reg_reg_insn(INSN_SHR_REG_REG, &VAR_ECX, &VAR_EAX));
	assert_emit_insn_2(0xd3, 0xeb, reg_reg_insn(INSN_SHR_REG_REG, &VAR_ECX, &VAR_EBX));
}

void test_emit_or_membase_reg(void)
{
	assert_emit_insn_3(0x0b, 0x45, 0x0c, membase_reg_insn(INSN_OR_MEMBASE_REG, &VAR_EBP, 0x0c, &VAR_EAX));
	assert_emit_insn_6(0x0b, 0x9d, 0xef, 0xbe, 0xad, 0xde, membase_reg_insn(INSN_OR_MEMBASE_REG, &VAR_EBP, 0xdeadbeef, &VAR_EBX));
}

void test_emit_and_membase_reg(void)
{
	assert_emit_insn_3(0x23, 0x45, 0x0c, membase_reg_insn(INSN_AND_MEMBASE_REG, &VAR_EBP, 0x0c, &VAR_EAX));
	assert_emit_insn_6(0x23, 0x9d, 0xef, 0xbe, 0xad, 0xde, membase_reg_insn(INSN_AND_MEMBASE_REG, &VAR_EBP, 0xdeadbeef, &VAR_EBX));
}

void test_emit_xor_membase_reg(void)
{
	assert_emit_insn_3(0x33, 0x45, 0x0c, membase_reg_insn(INSN_XOR_MEMBASE_REG, &VAR_EBP, 0x0c, &VAR_EAX));
	assert_emit_insn_6(0x33, 0x9d, 0xef, 0xbe, 0xad, 0xde, membase_reg_insn(INSN_XOR_MEMBASE_REG, &VAR_EBP, 0xdeadbeef, &VAR_EBX));
}

void test_emit_cmp_membase_reg(void)
{
	assert_emit_insn_3(0x3b, 0x45, 0x08, membase_reg_insn(INSN_CMP_MEMBASE_REG, &VAR_EBP, 0x08, &VAR_EAX));
}

void test_emit_cmp_imm_reg(void)
{
	assert_emit_insn_3(0x83, 0xf8, 0x04, imm_reg_insn(INSN_CMP_IMM_REG, 0x04, &VAR_EAX));
	assert_emit_insn_3(0x83, 0xfb, 0x08, imm_reg_insn(INSN_CMP_IMM_REG, 0x08, &VAR_EBX));
	assert_emit_insn_6(0x81, 0xfb, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(INSN_CMP_IMM_REG, 0xdeadbeef, &VAR_EBX));
}

static void assert_emit_prefixed_insn_5(unsigned char expected_prefix,
					unsigned char expected_1,
					unsigned char expected_2,
					unsigned char expected_3,
					unsigned char expected_4,
					unsigned char expected_5,
					struct insn *insn)
{
	if (expected_prefix)
		assert_emit_insn_6(expected_prefix, expected_1, expected_2, expected_3, expected_4, expected_5, insn);
	else
		assert_emit_insn_5(expected_1, expected_2, expected_3, expected_4, expected_5, insn);
}

static void assert_forward_branch(unsigned char expected_prefix,
				  unsigned char expected_opc,
				  enum insn_type insn_type)
{
	struct basic_block *bb;
	
	bb = alloc_basic_block(NULL, 0, 1);

	assert_emit_prefixed_insn_5(expected_prefix, expected_opc, 0x00, 0x00, 0x00, 0x00, branch_insn(insn_type, bb));

	free_basic_block(bb);
}

static void assert_prefixed_mem_insn_5(unsigned char expected_prefix,
				       unsigned char expected_1,
				       unsigned char expected_2,
				       unsigned char expected_3,
				       unsigned char expected_4,
				       unsigned char expected_5,
				       const char *actual)
{
	if (expected_prefix)
		assert_mem_insn_6(expected_prefix, expected_1, expected_2, expected_3, expected_4, expected_5, actual);
	else
		assert_mem_insn_5(expected_1, expected_2, expected_3, expected_4, expected_5, actual);
}

static void assert_emits_branch_target(unsigned char expected_prefix,
				       unsigned char expected_opc,
				       unsigned char expected_target_1,
				       unsigned char expected_target_2,
				       unsigned char expected_target_3,
				       unsigned char expected_target_4,
				       struct basic_block *target_bb,
				       enum insn_type insn_type)
{
	struct basic_block *branch_bb;
	struct buffer *buf;
	struct insn *insn;

	insn = branch_insn(insn_type, target_bb);
	branch_bb = alloc_basic_block(NULL, 1, 2);

	buf = alloc_buffer();
	emit_body(target_bb, buf);

	bb_add_insn(branch_bb, insn);
	emit_body(branch_bb, buf);

	if (expected_prefix)
		expected_target_1--;

	assert_prefixed_mem_insn_5(expected_prefix, expected_opc,
		expected_target_1, expected_target_2, expected_target_3,
		expected_target_4, buffer_ptr(buf) + insn->mach_offset);

	free_basic_block(branch_bb);
	free_buffer(buf);
}

static void
assert_emit_target_for_backward_branches(unsigned char expected_prefix,
					 unsigned char expected_opc,
					 enum insn_type insn_type)
{
	struct basic_block *target_bb;
	struct compilation_unit *cu;
	struct var_info *eax, *ebx;

	cu = alloc_compilation_unit(&method);
	eax = get_fixed_var(cu, REG_EAX);
	ebx = get_fixed_var(cu, REG_EBX);
	
	target_bb = get_basic_block(cu, 0, 1);

	bb_add_insn(target_bb, imm_reg_insn(INSN_ADD_IMM_REG, 0x01, eax));
	assert_emits_branch_target(expected_prefix, expected_opc, 0xf8, 0xff, 0xff, 0xff, target_bb, insn_type);

	bb_add_insn(target_bb, imm_reg_insn(INSN_ADD_IMM_REG, 0x02, ebx));
	assert_emits_branch_target(expected_prefix, expected_opc, 0xf5, 0xff, 0xff, 0xff, target_bb, insn_type);

	free_compilation_unit(cu);
}

static void
assert_adds_self_to_unresolved_list_for_forward_branches(
		unsigned char expected_prefix,
		unsigned char expected_opc,
		enum insn_type insn_type)
{
	struct basic_block *if_true;
	struct insn *insn;

	if_true = alloc_basic_block(NULL, 0, 1);
	insn = branch_insn(insn_type, if_true);

	assert_emit_prefixed_insn_5(expected_prefix, expected_opc, 0x00, 0x00, 0x00, 0x00, insn);

	assert_ptr_equals(insn, list_entry(if_true->backpatch_insns.next,
					   struct insn, branch_list_node));

	free_basic_block(if_true);
}

static void assert_backpatches_branches(unsigned char expected_prefix,
					unsigned char expected_opc,
					unsigned char expected_target,
					struct basic_block *branch_bb,
					struct basic_block *target_bb)
{
	struct buffer *buf;

	branch_bb->is_emitted = false;
	target_bb->is_emitted = false;

	buf = alloc_buffer();

	emit_body(branch_bb, buf);
	assert_prefixed_mem_insn_5(expected_prefix, expected_opc, 0x00, 0x00, 0x00, 0x00, buffer_ptr(buf));

	emit_body(target_bb, buf);
	assert_prefixed_mem_insn_5(expected_prefix, expected_opc, expected_target, 0x00, 0x00, 0x00, buffer_ptr(buf));

	free_buffer(buf);
}

static void
assert_backpatches_unresolved_branches_when_emitting_target(
		unsigned char expected_prefix,
		unsigned char expected_opc,
		enum insn_type insn_type)
{
	struct basic_block *target_bb, *branch_bb;
	struct compilation_unit *cu;
	struct var_info *eax;

	cu = alloc_compilation_unit(&method);
	eax = get_fixed_var(cu, REG_EAX);

	branch_bb = get_basic_block(cu, 0, 1);
	target_bb = get_basic_block(cu, 1, 2);

	bb_add_insn(branch_bb, branch_insn(insn_type, target_bb));
	assert_backpatches_branches(expected_prefix, expected_opc, 0x00, branch_bb, target_bb);

	bb_add_insn(branch_bb, imm_reg_insn(INSN_ADD_IMM_REG, 0x01, eax));
	assert_backpatches_branches(expected_prefix, expected_opc, 0x03, branch_bb, target_bb);

	free_compilation_unit(cu);
}

static void assert_emit_branch(unsigned char expected_prefix,
			       unsigned char expected_opc,
			       enum insn_type insn_type)
{
	assert_forward_branch(expected_prefix, expected_opc, insn_type);
	assert_emit_target_for_backward_branches(expected_prefix, expected_opc, insn_type);
	assert_adds_self_to_unresolved_list_for_forward_branches(expected_prefix, expected_opc, insn_type);
	assert_backpatches_unresolved_branches_when_emitting_target(expected_prefix, expected_opc, insn_type);
}

void test_emit_branches(void)
{
	assert_emit_branch(0x0f, 0x84, INSN_JE_BRANCH);
	assert_emit_branch(0x0f, 0x85, INSN_JNE_BRANCH);
	assert_emit_branch(0x00, 0xe9, INSN_JMP_BRANCH);
}
