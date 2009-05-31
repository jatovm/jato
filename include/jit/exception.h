#ifndef JATO_JIT_EXCEPTION_H
#define JATO_JIT_EXCEPTION_H

#include <stdbool.h>
#include <vm/vm.h>

struct compilation_unit;
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

/* This should be called only by JIT compiled native code */
unsigned char *throw_exception(struct compilation_unit *cu,
			       struct object *exception);
void throw_exception_from_signal(void *ctx, struct object *exception);
void unwind(void);

#endif /* JATO_JIT_EXCEPTION_H */
