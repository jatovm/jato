/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <system.h>
#include <assembler.h>
#include <basic-block.h>
#include <instruction.h>

static void assert_emit_mov_membase_reg(unsigned long src_disp,
					enum reg dest_reg,
					unsigned long modrm)
{
	struct basic_block *bb;
	unsigned char expected[] = { 0x8b, modrm, src_disp };
	unsigned char actual[3];

	bb = alloc_basic_block(0, 1);
	bb_insert_insn(bb, x86_op_membase_reg(MOV, REG_EBP, src_disp, dest_reg));
	assemble(bb, actual, ARRAY_SIZE(actual));
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
	
	free_basic_block(bb);
}

void test_emit_mov_membase_reg(void)
{
	assert_emit_mov_membase_reg(8, REG_EAX, 0x45);
	assert_emit_mov_membase_reg(12, REG_EAX, 0x45);
	assert_emit_mov_membase_reg(8, REG_EBX, 0x5D);
	assert_emit_mov_membase_reg(8, REG_ECX, 0x4D);
	assert_emit_mov_membase_reg(8, REG_EDX, 0x55);
}
