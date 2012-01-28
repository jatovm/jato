#include "jit/emit-code.h"

#include "jit/constant-pool.h"
#include "jit/basic-block.h"
#include "jit/lir-printer.h"
#include "jit/exception.h"

#include "vm/backtrace.h"
#include "vm/call.h"
#include "vm/itable.h"
#include "vm/stack-trace.h"
#include "vm/static.h"

#include "lib/buffer.h"

#include "arch/itable.h"
#include "arch/instruction.h"
#include "arch/encode.h"

void __attribute__((regparm(1)))
itable_resolver_stub_error(struct vm_method *method, struct vm_object *obj)
{
	assert(!"not implemented");
}

int lir_print(struct insn *insn, struct string *s)
{
	assert(!"not implemented");
}

void native_call(struct vm_method *method, void *target, unsigned long *args, union jvalue *result)
{
	assert(!"not implemented");
}

void *emit_itable_resolver_stub(struct vm_class *vmc, struct itable_entry **sorted_table, unsigned int nr_entries)
{
	assert(!"not implemented");
}

bool called_from_jit_trampoline(struct native_stack_frame *frame)
{
	assert(!"not implemented");
}

bool show_exe_function(void *addr, struct string *str)
{
	assert(!"not implemented");
}

int fixup_static_at(unsigned long addr)
{
	assert(!"not implemented");
}

void emit_trampoline(struct compilation_unit *cu,
		     void *call_target,
		     struct jit_trampoline *trampoline)
{
	struct buffer *buf = trampoline->objcode;

	jit_text_lock();
	buf->buf = jit_text_ptr();

	/* Store FP and LR */
	encode_stm(buf, 0b0100100000000000);

	/* Setup the frame pointer to point to the current frame */
	encode_setup_fp(buf, 4);

	/* Pass the address of cu to magic_trampoline in R0 and call it */
	encode_setup_trampoline(buf, (unsigned long)cu, (unsigned long)call_target);

	/* Call the function whose address is is returned by magic trampoline */
	encode_emit_branch_link(buf);
	/*
	 * Setup the stack frame to the previous value
	 * before entering the frame
	 */
	encode_restore_sp(buf, 4);
	encode_ldm(buf, 0b1000100000000000);

	jit_text_reserve(buffer_offset(buf));
	jit_text_unlock();
}

void emit_jni_trampoline(struct buffer *b, struct vm_method *vm, void *v)
{
	assert(!"not implemented");
}

void fixup_direct_calls(struct jit_trampoline *trampoline, unsigned long target)
{
}

void emit_unlock(struct buffer *buffer, struct vm_object *vo)
{
	assert(!"not implemented");
}

void emit_unlock_this(struct buffer *buffer, unsigned long frame_size)
{
	assert(!"not implemented");
}
static inline void write_imm24(struct buffer *buf, struct insn *insn,
				unsigned long imm, unsigned long offset)
{
	unsigned char *buffer;
	union {
		int val;
		unsigned char b[4];
	} imm_buf;

	buffer = buf->buf;
	imm_buf.val = imm;

	buffer[offset] = imm_buf.b[0];
	buffer[offset + 1] = imm_buf.b[1];
	buffer[offset + 2] = imm_buf.b[2];
}

/*
 * This function calculates the pc relative address of target
 * and emits the backpatched branch insns.
 */
void backpatch_branch_target(struct buffer *buf, struct insn *insn,
				    unsigned long target_offset)
{
	unsigned long backpatch_offset;
	long relative_addr;

	relative_addr = branch_rel_addr(insn, target_offset);
	relative_addr = relative_addr >> 2;
	backpatch_offset = insn->mach_offset;

	write_imm24(buf, insn, relative_addr, backpatch_offset);
}

void emit_nop(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_prolog(struct buffer *buf, struct stack_frame *frame,
					unsigned long frame_size)
{
	/* Store FP and LR, LR is stored unconditionally */
	encode_stm(buf, 0b0100100000000000);
	/* Setup the frame pointer to point to the current frame */
	encode_setup_fp(buf, 4);
	/* Store R4-R10 unconditionally */
	encode_stm(buf, 0b0000011111110000);
	/* Make space for current Frame */
	encode_sub_sp(buf, frame_size);
	/* Store the arguments passed in R0-R3 */
	encode_store_args(buf, frame);
}

void emit_epilog(struct buffer *buf)
{
	/*
	 * Setup the stack frame to the previous value
	 * before entering the frame
	 */
	encode_restore_sp(buf, 4);
	/*
	 * Load all the registers stored on the frame
	 * It loads the stored LR, to PC directly so that control
	 * returns to the calling function.
	 */
	encode_ldm(buf, 0b1000111111110000);
}

void emit_lock(struct buffer *buf, struct vm_object *vo)
{
	assert(!"not implemented");
}

void emit_lock_this(struct buffer *buf, unsigned long frame_size)
{
	assert(!"not implemented");
}

void emit_unwind(struct buffer *buf)
{
}

void emit_trace_invoke(struct buffer *buf, struct compilation_unit *cu)
{
	assert(!"not implemented");
}

long branch_rel_addr(struct insn *insn, unsigned long target_offset)
{
	long addr;
	addr = target_offset - insn->mach_offset - PC_RELATIVE_OFFSET;

	return addr;
}

/*
 * This function emits the branch insns.
 * we first check if there is any resolution block is attached
 * to the CFG Edge. If there is a resolution block we backpatch
 * that insn. Else we check if the target basic_block is emitted
 * or not. If its emitted then we just have to find the PC
 * relative address of the target basic block but if its not
 * emitted then we have to backpatch that branch insn.
 */
long emit_branch(struct insn *insn, struct basic_block *bb)
{
	struct basic_block *target_bb;
	long addr = 0;
	int idx;

	target_bb = insn->operand.branch_target;

	if (!bb)
		idx = -1;
	else
		idx = bb_lookup_successor_index(bb, target_bb);

	if (idx >= 0 && branch_needs_resolution_block(bb, idx)) {
		insn->flags |= INSN_FLAG_BACKPATCH_RESOLUTION;
		insn->operand.resolution_block = &bb->resolution_blocks[idx];
	} else if (target_bb->is_emitted) {
		addr = branch_rel_addr(insn, target_bb->mach_offset);
	} else
		insn->flags |= INSN_FLAG_BACKPATCH_BRANCH;

	return addr;

}

void emit_insn(struct buffer *buf, struct basic_block *bb, struct insn *insn)
{
	insn->mach_offset = buffer_offset(buf);

	insn_encode(insn, buf, bb);
}
