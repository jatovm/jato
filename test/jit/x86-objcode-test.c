/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <x86-objcode.h>
#include <jit/basic-block.h>
#include <jit/instruction.h>
#include <jit/statement.h>
#include <vm/list.h>
#include <vm/system.h>

#include <libharness.h>

static void assert_emit_insn(unsigned char *expected,
			     unsigned long expected_size,
			     struct insn *insn)
{
	struct insn_sequence is;
	struct basic_block *bb;
	unsigned char actual[expected_size];

	bb = alloc_basic_block(NULL, 0, 1);
	bb_add_insn(bb, insn);

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

static void assert_mem_insn_2(unsigned char expected_opc,
			      unsigned char expected_extra, const char *actual)
{
	unsigned char expected[] = { expected_opc, expected_extra };

	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
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

static void assert_emit_insn_6(unsigned char opcode, unsigned char modrm,
			       unsigned char b1, unsigned char b2,
			       unsigned char b3, unsigned char b4,
			       struct insn *insn)
{
	unsigned char expected[] = { opcode, modrm, b1, b2, b3, b4 };

	assert_emit_insn(expected, ARRAY_SIZE(expected), insn);
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
	unsigned char actual[3];
	struct insn_sequence is;

	init_insn_sequence(&is, actual, 3);
	x86_emit_prolog(&is, 0);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_prolog_has_locals(void)
{
	unsigned char expected[] = { 0x55, 0x89, 0xe5, 0x81, 0xec, 0x80, 0x00, 0x00, 0x00 };
	unsigned char actual[ARRAY_SIZE(expected)];
	struct insn_sequence is;

	init_insn_sequence(&is, actual, ARRAY_SIZE(expected));
	x86_emit_prolog(&is, 0x80);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_epilog_no_locals(void)
{
	unsigned char expected[] = { 0x5d, 0xc3 };
	unsigned char actual[2];
	struct insn_sequence is;
	
	init_insn_sequence(&is, actual, 2);
	x86_emit_epilog(&is, 0);
	assert_mem_equals(expected, actual, ARRAY_SIZE(expected));
}

void test_emit_epilog_has_locals(void)
{
	unsigned char expected[] = { 0xc9, 0xc3 };
	unsigned char actual[ARRAY_SIZE(expected)];
	struct insn_sequence is;
	
	init_insn_sequence(&is, actual, ARRAY_SIZE(expected));
	x86_emit_epilog(&is, 0x80);
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

	bb = alloc_basic_block(NULL, 0, 1);
	bb_add_insn(bb, imm_insn(OPC_PUSH, imm));
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

	bb = alloc_basic_block(NULL, 0, 1);
	bb_add_insn(bb, rel_insn(OPC_CALL, (unsigned long) call_target));
	
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

void test_emit_indirect_call(void)
{
	assert_emit_insn_2(0xff, 0xe0, reg_insn(OPC_JMP, REG_EAX));
	assert_emit_insn_2(0xff, 0xe3, reg_insn(OPC_JMP, REG_EBX));
	assert_emit_insn_2(0xff, 0xe1, reg_insn(OPC_JMP, REG_ECX));
}

void test_emit_mov_imm(void)
{
	assert_emit_insn_5(0xb8, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(OPC_MOV, 0xdeadbeef, REG_EAX));
	assert_emit_insn_5(0xbb, 0xbe, 0xba, 0xfe, 0xca, imm_reg_insn(OPC_MOV, 0xcafebabe, REG_EBX));
	assert_emit_insn_5(0xb9, 0xbe, 0xba, 0xfe, 0xca, imm_reg_insn(OPC_MOV, 0xcafebabe, REG_ECX));
	assert_emit_insn_5(0xba, 0xbe, 0xba, 0xfe, 0xca, imm_reg_insn(OPC_MOV, 0xcafebabe, REG_EDX));
}

void test_emit_mov_imm_disp(void)
{
	assert_emit_insn_6(0xc7, 0x00, 0xbe, 0xba, 0xfe, 0xca, imm_membase_insn(OPC_MOV, 0xcafebabe, REG_EAX, 0));
	assert_emit_insn_6(0xc7, 0x03, 0xef, 0xbe, 0xad, 0xde, imm_membase_insn(OPC_MOV, 0xdeadbeef, REG_EBX, 0));
	assert_emit_insn_7(0xc7, 0x40, 0x08, 0xbe, 0xba, 0xfe, 0xca, imm_membase_insn(OPC_MOV, 0xcafebabe, REG_EAX, 0x08));
	assert_emit_insn_7(0xc7, 0x43, 0x0c, 0xbe, 0xba, 0xfe, 0xca, imm_membase_insn(OPC_MOV, 0xcafebabe, REG_EBX, 0x0c));
}

void test_emit_mov_disp_reg(void)
{
	assert_emit_insn_3(0x8b, 0x45, 0x08, membase_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_EAX));
	assert_emit_insn_3(0x8b, 0x45, 0x0c, membase_reg_insn(OPC_MOV, REG_EBP, 0x0c, REG_EAX));

	assert_emit_insn_3(0x8b, 0x5D, 0x08, membase_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_EBX));
	assert_emit_insn_3(0x8b, 0x4D, 0x08, membase_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_ECX));
	assert_emit_insn_3(0x8b, 0x55, 0x08, membase_reg_insn(OPC_MOV, REG_EBP, 0x08, REG_EDX));
	assert_emit_insn_4(0x8b, 0x44, 0x24, 0x00, membase_reg_insn(OPC_MOV, REG_ESP, 0, REG_EAX));

	assert_emit_insn_6(0x8b, 0x80, 0x50, 0x02, 0x00, 0x00, membase_reg_insn(OPC_MOV, REG_EAX, 0x250, REG_EAX));
}

void test_emit_mov_reg_membase(void)
{
	assert_emit_insn_3(0x89, 0x43, 0x04, reg_membase_insn(OPC_MOV, REG_EAX, REG_EBX, 0x04));
	assert_emit_insn_3(0x89, 0x41, 0x04, reg_membase_insn(OPC_MOV, REG_EAX, REG_ECX, 0x04));
	assert_emit_insn_3(0x89, 0x41, 0x08, reg_membase_insn(OPC_MOV, REG_EAX, REG_ECX, 0x08));
	assert_emit_insn_6(0x89, 0x98, 0xef, 0xbe, 0xad, 0xde, reg_membase_insn(OPC_MOV, REG_EBX, REG_EAX, 0xdeadbeef));
}

void test_emit_add_disp_reg(void)
{
	assert_emit_insn_3(0x03, 0x45, 0x04, membase_reg_insn(OPC_ADD, REG_EBP, 0x04, REG_EAX));
}

void test_emit_add_imm(void)
{
	assert_emit_insn_3(0x83, 0xc0, 0x04, imm_reg_insn(OPC_ADD, 0x04, REG_EAX));
	assert_emit_insn_3(0x83, 0xc3, 0x08, imm_reg_insn(OPC_ADD, 0x08, REG_EBX));
	assert_emit_insn_6(0x81, 0xc0, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(OPC_ADD, 0xdeadbeef, REG_EAX));
}

void test_emit_cmp_membase_reg(void)
{
	assert_emit_insn_3(0x3b, 0x45, 0x08, membase_reg_insn(OPC_CMP, REG_EBP, 0x08, REG_EAX));
}

void test_emit_cmp_imm_reg(void)
{
	assert_emit_insn_3(0x83, 0xf8, 0x04, imm_reg_insn(OPC_CMP, 0x04, REG_EAX));
	assert_emit_insn_3(0x83, 0xfb, 0x08, imm_reg_insn(OPC_CMP, 0x08, REG_EBX));
	assert_emit_insn_6(0x81, 0xfb, 0xef, 0xbe, 0xad, 0xde, imm_reg_insn(OPC_CMP, 0xdeadbeef, REG_EBX));
}

static void assert_forward_branch(unsigned char expected_opc, enum insn_opcode actual_opc)
{
	struct basic_block *bb = alloc_basic_block(NULL, 0, 1);

	assert_emit_insn_2(expected_opc, 0x00, branch_insn(actual_opc, bb));

	free_basic_block(bb);
}

void test_should_use_zero_as_target_branch_for_forward_branches(void)
{
	assert_forward_branch(0x74, OPC_JE);
	assert_forward_branch(0xeb, OPC_JMP);
}

static void assert_emits_branch_target(unsigned char expected_target,
				       struct basic_block *target_bb)
{
	struct insn *insn;
	struct basic_block *branch_bb;
	struct insn_sequence is;
	char code[16];

	insn = branch_insn(OPC_JE, target_bb);
	branch_bb = alloc_basic_block(NULL, 1, 2);

	init_insn_sequence(&is, code, 16);

	x86_emit_obj_code(target_bb, &is);

	bb_add_insn(branch_bb, insn);
	x86_emit_obj_code(branch_bb, &is);

	assert_mem_insn_2(0x74, expected_target, code + insn->offset);

	free_basic_block(branch_bb);
}

void test_should_emit_target_for_backward_branches(void)
{
	struct basic_block *target_bb = alloc_basic_block(NULL, 0, 1);

	bb_add_insn(target_bb, imm_reg_insn(OPC_ADD, 0x01, REG_EAX));
	assert_emits_branch_target(0xfb, target_bb);

	bb_add_insn(target_bb, imm_reg_insn(OPC_ADD, 0x02, REG_EBX));
	assert_emits_branch_target(0xf8, target_bb);

	free_basic_block(target_bb);
}

void test_should_add_self_to_unresolved_list_for_forward_branches(void)
{
	struct basic_block *if_true;
	struct insn *insn;

	if_true = alloc_basic_block(NULL, 0, 1);
	insn = branch_insn(OPC_JE, if_true);

	assert_emit_insn_2(0x74, 0x00, insn);

	assert_ptr_equals(insn, list_entry(if_true->backpatch_insns.next,
					   struct insn, branch_list_node));

	free_basic_block(if_true);
}

static void assert_backpatches_branches(unsigned char expected_target,
					struct basic_block *branch_bb,
					struct basic_block *target_bb)
{
	char code[16];
	struct insn_sequence is;

	init_insn_sequence(&is, code, 16);

	x86_emit_obj_code(branch_bb, &is);
	assert_mem_insn_2(0x74, 0x00, code);

	x86_emit_obj_code(target_bb, &is);
	assert_mem_insn_2(0x74, expected_target, code);
}

void test_should_backpatch_unresolved_branches_when_emitting_target(void)
{
	struct basic_block *target_bb, *branch_bb;

	branch_bb = alloc_basic_block(NULL, 0, 1);
	target_bb = alloc_basic_block(NULL, 1, 2);

	bb_add_insn(branch_bb, branch_insn(OPC_JE, target_bb));
	assert_backpatches_branches(0x00, branch_bb, target_bb);

	bb_add_insn(branch_bb, imm_reg_insn(OPC_ADD, 0x01, REG_EAX));
	assert_backpatches_branches(0x03, branch_bb, target_bb);

	free_basic_block(branch_bb);
	free_basic_block(target_bb);
}
