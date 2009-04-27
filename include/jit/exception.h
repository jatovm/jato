#ifndef _JIT_EXCEPTION_H
#define _JIT_EXCEPTION_H

#include <jit/compilation-unit.h>
#include <vm/vm.h>

struct exception_table_entry *exception_find_entry(struct methodblock *,
						   unsigned long);

static inline bool exception_covers(struct exception_table_entry *eh,
				    unsigned long offset)
{
	return eh->start_pc <= offset && offset < eh->end_pc;
}

#endif
