#ifndef _JIT_EXCEPTION_H
#define _JIT_EXCEPTION_H

#include <jit/compilation-unit.h>
#include <vm/vm.h>
#include <arch/stack-frame.h>

struct jit_stack_frame;

struct exception_table_entry *exception_find_entry(struct methodblock *,
						   unsigned long);

static inline bool exception_covers(struct exception_table_entry *eh,
				    unsigned long offset)
{
	return eh->start_pc <= offset && offset < eh->end_pc;
}

unsigned char *throw_exception_from(struct compilation_unit *cu,
				    struct jit_stack_frame *frame,
				    unsigned char *native_ptr,
				    struct object *exception);
int insert_exception_spill_insns(struct compilation_unit *cu);

#endif
