/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <x86-objcode.h>
#include <basic-block.h>
#include <instruction.h>

#include <vm/system.h>

#include <libharness.h>

static void assert_emit_insn(unsigned char *expected,
			     unsigned long expected_size,
			     struct insn *insn)
{
	struct insn_sequence is;
	struct basic_block *bb;
	unsigned char actual[expected_size];

	bb = alloc_basic_block(0, 1);
	bb_insert_insn(bb, insn);

	init_insn_sequence(&is, actual, expected_size);
	x86_emit_obj_code(bb, &is);
	assert_mem_equals(expected, actual, expected_size);
	
	free_basic_block(bb);
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

static void assert_emit_insn_3(unsigned char opcode, unsigned long modrm,
			       unsigned char extra, struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, extra };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
}

void test_emit_prolog(void)
{
	unsigned char expected[] = { 0x55, 0x89, 0xe5 };
	unsigned char actual[3];
	struct insn_sequence is;

	init_insn_sequence(&is, actual, 3);
	x86_emit_prolog(&is);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_epilog(void)
{
	unsigned char expected[] = { 0x5d, 0xc3 };
	unsigned char actual[2];
	struct insn_sequence is;
	
	init_insn_sequence(&is, actual, 2);
	x86_emit_epilog(&is);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

static void assert_emit_push_imm32(unsigned long imm)
{
	struct basic_block *bb;
	unsigned char expected[] = {
		0x68,
		(imm & 0x000000ffUL),
		(imm & 0x0000ff00UL) >>  8,
		(imm & 0x00ff0000UL) >> 16,
		(imm & 0xff000000UL) >> 24,
	};
	unsigned char actual[5];
	struct insn_sequence is;

	bb = alloc_basic_block(0, 1);
	bb_insert_insn(bb, imm_insn(OPC_PUSH, imm));
	init_insn_sequence(&is, actual, ARRAY_SIZE(actual));

	x86_emit_obj_code(bb, &is);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));

	free_basic_block(bb);
}

void test_emit_push_imm32(void)
{
	assert_emit_push_imm32(0x0);
	assert_emit_push_imm32(0xdeadbeef);
}

void test_emit_push_reg(void)
{
	assert_emit_insn_1(0x50, reg_insn(OPC_PUSH, REG_EAX));
	assert_emit_insn_1(0x53, reg_insn(OPC_PUSH, REG_EBX));
	assert_emit_insn_1(0x51, reg_insn(OPC_PUSH, REG_ECX));
	assert_emit_insn_1(0x52, reg_insn(OPC_PUSH, REG_EDX));
	assert_emit_insn_1(0x55, reg_insn(OPC_PUSH, REG_EBP));
	assert_emit_insn_1(0x54, reg_insn(OPC_PUSH, REG_ESP));
}

static void assert_emit_call(void *call_target,
			     void *code,
			     unsigned long code_size)
{
	struct basic_block *bb;
	signed long disp = call_target - code - 5;
	unsigned char expected[] = {
		0xe8,
		(disp & 0x000000ffUL),
		(disp & 0x0000ff00UL) >>  8,
		(disp & 0x00ff0000UL) >> 16,
		(disp & 0xff000000UL) >> 24,
	};
	struct insn_sequence is;

	bb = alloc_basic_block(0, 1);
	bb_insert_insn(bb, rel_insn(OPC_CALL, (unsigned long) call_target));
	
	init_insn_sequence(&is, code, code_size);
	x86_emit_obj_code(bb, &is);
	assert_mem_equals(expected, code, ARRAY_SIZE(expected));

	free_basic_block(bb);
}

void test_emit_call_backward(void)
{
	unsigned char before_code[3];
	unsigned char code[5];
	assert_emit_call(before_code, code, ARRAY_SIZE(code));
}

void test_emit_call_forward(void)
{
	unsigned char code[5];
	unsigned char after_code[3];
	assert_emit_call(after_code, code, ARRAY_SIZE(code));
}

static void assert_emit_add_imm8_reg(unsigned char imm8, unsigned char modrm, enum reg reg)
{
	unsigned char expected[] = { 0x83, modrm, imm8 };
	unsigned char actual[3];
	struct insn_sequence is;

	init_insn_sequence(&is, actual, ARRAY_SIZE(actual));
	x86_emit_add_imm8_reg(&is, imm8, reg);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_add_imm8_reg(void)
{
	assert_emit_add_imm8_reg(0x04, 0xc0, REG_EAX);
	assert_emit_add_imm8_reg(0x04, 0xc3, REG_EBX);
	assert_emit_add_imm8_reg(0x04, 0xc1, REG_ECX);
	assert_emit_add_imm8_reg(0x04, 0xc2, REG_EDX);
	assert_emit_add_imm8_reg(0x04, 0xc4, REG_ESP);
	assert_emit_add_imm8_reg(0x08, 0xc4, REG_ESP);
	assert_emit_add_imm8_reg(0x04, 0xc5, REG_EBP);
}

static void assert_emit_indirect_jump_reg(unsigned char modrm, enum reg reg)
{
	unsigned char expected[] = { 0xff, modrm };
	unsigned char actual[2];
	struct insn_sequence is;

	init_insn_sequence(&is, actual, ARRAY_SIZE(actual));
	x86_emit_indirect_jump_reg(&is, reg);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_indirect_jump_reg(void)
{
	assert_emit_indirect_jump_reg(0xe0, REG_EAX);
	assert_emit_indirect_jump_reg(0xe3, REG_EBX);
}

void test_emit_mov_disp_reg(void)
{
	assert_emit_insn_3(0x8b, 0x45, 0x08, disp_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_EAX));
	assert_emit_insn_3(0x8b, 0x45, 0x0c, disp_reg_insn(OPC_MOV, REG_EBP, 0x0c, REG_EAX));

	assert_emit_insn_3(0x8b, 0x5D, 0x08, disp_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_EBX));
	assert_emit_insn_3(0x8b, 0x4D, 0x08, disp_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_ECX));
	assert_emit_insn_3(0x8b, 0x55, 0x08, disp_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_EDX));
}

void test_emit_add_disp_reg(void)
{
	assert_emit_insn_3(0x03, 0x45, 0x04, disp_reg_insn(OPC_ADD, REG_EBP, 0x04, REG_EAX));
}

void test_emit_cmp_disp_reg(void)
{
	assert_emit_insn_3(0x3b, 0x45, 0x08, disp_reg_insn(OPC_CMP, REG_EBP, 0x08, REG_EAX));
}

void test_emit_je_rel(void)
{
	assert_emit_insn_2(0x74, 0x01, rel_insn(OPC_JE, 0x01));
}

void test_emit_insn_3(void)
{
	assert_emit_insn_3(0x83, 0xc0, 0x01, imm_reg_insn(OPC_ADD, 0x01, REG_EAX));
}
