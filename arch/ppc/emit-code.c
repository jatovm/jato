/*
 * Copyright (c) 2011  Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "jit/emit-code.h"

#include "jit/basic-block.h"
#include "jit/compiler.h"

#include "arch/instruction.h"
#include "arch/encode.h"

#include "lib/buffer.h"

#include <stdlib.h>

static void emit(struct buffer *b, unsigned long insn)
{
	buffer_write_be32(b, insn);
}

void itable_resolver_stub_error(struct vm_method *method, struct vm_object *obj)
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

void
emit_trampoline(struct compilation_unit *cu, void *target_addr, struct jit_trampoline *t)
{
	struct buffer *b = t->objcode;

	jit_text_lock();

	b->buf = jit_text_ptr();

	/* Pass pointer to 'struct compilation_unit' as first argument */
	emit(b, lis(3, ptr_high(cu)));
	emit(b, ori(3, 3, ptr_low(cu)));

	/* Then call 'target_addr' */
	emit(b, lis(0, ptr_high(target_addr)));
	emit(b, ori(0, 0, ptr_low(target_addr)));
	emit(b, mtctr(0));
	emit(b, bctrl());

	/* Finally jump to the compiled method */
	emit(b, mtctr(3));
	emit(b, bctr());

	jit_text_reserve(buffer_offset(b));

	jit_text_unlock();
}

void emit_jni_trampoline(struct buffer *b, struct vm_method *vm, void *v)
{
	assert(!"not implemented");
}

void fixup_direct_calls(struct jit_trampoline *trampoline, unsigned long target)
{
	assert(!"not implemented");
}

void emit_unlock(struct buffer *buffer, struct vm_object *vo)
{
	assert(!"not implemented");
}

void emit_unlock_this(struct buffer *buffer)
{
	assert(!"not implemented");
}

void backpatch_branch_target(struct buffer *buf, struct insn *insn,
				    unsigned long target_offset)
{
	assert(!"not implemented");
}

void emit_nop(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_prolog(struct buffer *buf, struct stack_frame *frame,
					unsigned long frame_size)
{
	assert(!"not implemented");
}

void emit_epilog(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_lock(struct buffer *buf, struct vm_object *vo)
{
	assert(!"not implemented");
}

void emit_lock_this(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_unwind(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_trace_invoke(struct buffer *buf, struct compilation_unit *cu)
{
	assert(!"not implemented");
}

long branch_rel_addr(struct insn *insn, unsigned long target_offset)
{
	assert(!"not implemented");
}

long emit_branch(struct insn *insn, struct basic_block *bb)
{
	assert(!"not implemented");
}

void emit_insn(struct buffer *buf, struct basic_block *bb, struct insn *insn)
{
	assert(!"not implemented");
}
