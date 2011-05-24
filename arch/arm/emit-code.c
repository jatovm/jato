#include "jit/emit-code.h"

#include "arch/instruction.h"
#include "jit/basic-block.h"
#include "lib/buffer.h"
#include "arch/itable.h"
#include "jit/lir-printer.h"
#include "vm/backtrace.h"
#include "jit/exception.h"
#include "vm/call.h"
#include "vm/itable.h"
#include "vm/stack-trace.h"
#include "vm/static.h"
#include "jit/emit-code.h"

void emit_insn(struct buffer *buf, struct basic_block *bb, struct insn *insn)
{
	assert(!"not implemented");
}

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

void show_function(void *addr)
{
	assert(!"not implemented");
}

int fixup_static_at(unsigned long addr)
{
	assert(!"not implemented");
}

void emit_trampoline(struct compilation_unit *cu, void *v, struct jit_trampoline *jt)
{
	assert(!"not implemented");
}

void emit_jni_trampoline(struct buffer *b, struct vm_method *vm, void *v)
{
	assert(!"not implemented");
}

void fixup_direct_calls(struct jit_trampoline *trampoline, unsigned long target)
{
	assert(!"not implemented");
}

int select_instructions(struct compilation_unit *cu)
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

void emit_prolog(struct buffer *buf, unsigned long l)
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

void emit_epilog(struct buffer *buf)
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
