/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <x86-objcode.h>
#include <basic-block.h>
#include <instruction.h>

#include <vm/system.h>

#include <libharness.h>

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
	unsigned char expected[] = {
		0x68,
		(imm & 0x000000ffUL),
		(imm & 0x0000ff00UL) >>  8,
		(imm & 0x00ff0000UL) >> 16,
		(imm & 0xff000000UL) >> 24,
	};
	unsigned char actual[5];
	struct insn_sequence is;

	init_insn_sequence(&is, actual, ARRAY_SIZE(actual));
	x86_emit_push_imm32(&is, imm);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_push_imm32(void)
{
	assert_emit_push_imm32(0x0);
	assert_emit_push_imm32(0xdeadbeef);
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

static void assert_emit_op_disp_reg(enum insn_opcode insn_opcode,
				       enum reg src_base_reg,
				       unsigned long src_disp,
				       enum reg dest_reg,
				       unsigned char opcode,
				       unsigned long modrm)
{
	struct basic_block *bb;
	unsigned char expected[] = { opcode, modrm, src_disp };
	unsigned char actual[3];
	struct insn_sequence is;

	bb = alloc_basic_block(0, 1);
	bb_insert_insn(bb, disp_reg_insn(insn_opcode, src_base_reg, src_disp, dest_reg));

	init_insn_sequence(&is, actual, 3);
	x86_emit_obj_code(bb, &is);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
	
	free_basic_block(bb);
}

void test_emit_mov_disp_reg(void)
{
	assert_emit_op_disp_reg(OPC_MOV, REG_EBP,  8, REG_EAX, 0x8b, 0x45);
	assert_emit_op_disp_reg(OPC_MOV, REG_EBP, 12, REG_EAX, 0x8b, 0x45);
	assert_emit_op_disp_reg(OPC_MOV, REG_EBP,  8, REG_EBX, 0x8b, 0x5D);
	assert_emit_op_disp_reg(OPC_MOV, REG_EBP,  8, REG_ECX, 0x8b, 0x4D);
	assert_emit_op_disp_reg(OPC_MOV, REG_EBP,  8, REG_EDX, 0x8b, 0x55);
}

void test_emit_add_disp_reg(void)
{
	assert_emit_op_disp_reg(OPC_ADD, REG_EBP, 4, REG_EAX, 0x03, 0x45);
}
