/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <system.h>
#include <x86-objcode.h>
#include <basic-block.h>
#include <instruction.h>

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

static void assert_emit_op_membase_reg(enum insn_opcode insn_opcode,
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
	bb_insert_insn(bb, x86_op_membase_reg(insn_opcode, src_base_reg, src_disp, dest_reg));

	init_insn_sequence(&is, actual, 3);
	x86_emit_obj_code(bb, &is);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
	
	free_basic_block(bb);
}

void test_emit_mov_membase_reg(void)
{
	assert_emit_op_membase_reg(MOV, REG_EBP,  8, REG_EAX, 0x8b, 0x45);
	assert_emit_op_membase_reg(MOV, REG_EBP, 12, REG_EAX, 0x8b, 0x45);
	assert_emit_op_membase_reg(MOV, REG_EBP,  8, REG_EBX, 0x8b, 0x5D);
	assert_emit_op_membase_reg(MOV, REG_EBP,  8, REG_ECX, 0x8b, 0x4D);
	assert_emit_op_membase_reg(MOV, REG_EBP,  8, REG_EDX, 0x8b, 0x55);
}

void test_emit_add_membase_reg(void)
{
	assert_emit_op_membase_reg(ADD, REG_EBP, 4, REG_EAX, 0x03, 0x45);
}
