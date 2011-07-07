#include "arch/encode.h"

#include "arch/instruction.h"
#include "arch/constant-pool.h"

#include "jit/use-position.h"
#include "jit/emit-code.h"
#include "jit/vars.h"

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
