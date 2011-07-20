#include "arch/encode.h"

#include "arch/instruction.h"
#include "arch/constant-pool.h"

#include "jit/use-position.h"
#include "jit/emit-code.h"
#include "jit/vars.h"
#include "jit/instruction.h"

#include "lib/buffer.h"

#include "libharness.h"

#include <stdint.h>

uint32_t read_mem32(struct buffer *buffer)
{
	return *(buffer->buf) | (*(buffer->buf+1) << 8) |
			(*(buffer->buf+2) << 16) | (*(buffer->buf+3) << 24);
}
void test_emit_reg_imm_insn(void)
{
	struct buffer *buffer = alloc_buffer();
	struct insn insn = {};
	struct live_interval interval = {.reg = MACH_REG_R4};
	uint32_t encoded_insn;

	insn.type = INSN_MOV_REG_IMM;

	insn.dest.type = OPERAND_REG;
	insn.dest.reg.interval = &interval;

	insn.src.type = OPERAND_IMM;
	insn.src.imm = 46;

	emit_insn(buffer, NULL, &insn);

	encoded_insn = read_mem32(buffer);

	assert_int_equals(0xE3A0402E, encoded_insn);

	free_buffer(buffer);
}

void test_emit_reg_memlocal_insn(void)
{
	struct stack_frame *frame = alloc_stack_frame(3, 6);
	struct buffer *buffer = alloc_buffer();
	struct insn insn = {};
	struct live_interval interval = {.reg = MACH_REG_R4};
	struct stack_slot *slot = get_local_slot(frame, 2);
	uint32_t encoded_insn;

	insn.type = INSN_LOAD_REG_MEMLOCAL;

	insn.dest.type = OPERAND_REG;
	insn.dest.reg.interval = &interval;

	insn.src.type = OPERAND_MEMLOCAL;
	insn.src.slot = slot;

	emit_insn(buffer, NULL, &insn);

	encoded_insn = read_mem32(buffer);

	assert_int_equals(0xE51B4038, encoded_insn);

	free_buffer(buffer);
}

void test_emit_reg_reg_insn(void)
{
	struct buffer *buffer = alloc_buffer();
	struct insn insn = {};
	struct live_interval interval1 = {.reg = MACH_REG_R4};
	struct live_interval interval2 = {.reg = MACH_REG_R7};
	uint32_t encoded_insn;

	insn.type = INSN_MOV_REG_REG;

	insn.dest.type = OPERAND_REG;
	insn.dest.reg.interval = &interval1;

	insn.src.type = OPERAND_REG;
	insn.src.reg.interval = &interval2;

	emit_insn(buffer, NULL, &insn);

	encoded_insn = read_mem32(buffer);

	assert_int_equals(0xE1A04007, encoded_insn);

	free_buffer(buffer);
}

void test_emit_uncond_branch_insn(void)
{
	struct buffer *buffer = alloc_buffer();
	struct compilation_unit *cu = stub_compilation_unit_alloc();
	struct basic_block *bb1 = alloc_basic_block(cu, 1, 20);
	struct basic_block *bb2 = alloc_basic_block(cu, 1, 20);
	struct insn insn1 = {};
	struct insn insn2 = {};
	struct insn *insn, *next;
	uint32_t encoded_insn;

	insn1.type = INSN_UNCOND_BRANCH;
	insn1.operand.type = OPERAND_REG;
	insn1.operand.branch_target = bb2;

	insn2.type = INSN_UNCOND_BRANCH;
	insn2.operand.type = OPERAND_REG;
	insn2.operand.branch_target = bb1;

	bb_add_insn(bb1, &insn1);
	bb_add_insn(bb2, &insn2);

	int err = bb_add_successor(bb1, bb2);
	err = bb_add_successor(bb2, bb1);

	unsigned long rb_size = sizeof(struct resolution_block) * bb1->nr_successors;
		bb1->resolution_blocks = malloc(rb_size);
	rb_size = sizeof(struct resolution_block) * bb2->nr_successors;
		bb2->resolution_blocks = malloc(rb_size);
	resolution_block_init(&bb1->resolution_blocks[0]);
	resolution_block_init(&bb2->resolution_blocks[0]);

	bb1->mach_offset = buffer_offset(buffer);
	bb1->is_emitted = true;
	for_each_insn(insn, &bb1->insn_list) {
		emit_insn(buffer, bb1, insn);
	}
	encoded_insn = read_mem32(buffer);
	assert_int_equals(0xEA000000, encoded_insn);

	bb2->mach_offset = buffer_offset(buffer);
	bb2->is_emitted = true;
	list_for_each_entry_safe(insn, next, &bb2->backpatch_insns, branch_list_node) {
		backpatch_branch_target(buffer, insn, bb2->mach_offset);
		list_del(&insn->branch_list_node);
	}
	for_each_insn(insn, &bb2->insn_list) {
		emit_insn(buffer, bb2, insn);
	}

	encoded_insn = read_mem32(buffer);
	assert_int_equals(0xEAFFFFFF, encoded_insn);
	encoded_insn = *(buffer->buf+4) | (*(buffer->buf+5) << 8) |
			(*(buffer->buf+6) << 16) | (*(buffer->buf+7) << 24);
	assert_int_equals(0xEAFFFFFD, encoded_insn);

	free_buffer(buffer);
}
