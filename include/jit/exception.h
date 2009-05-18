#ifndef _JIT_EXCEPTION_H
#define _JIT_EXCEPTION_H

#include <cafebabe/code_attribute.h>

#include <jit/compilation-unit.h>

#include <vm/method.h>
#include <vm/vm.h>

struct cafebabe_code_attribute_exception *
exception_find_entry(struct vm_method *, unsigned long);

static inline bool
exception_covers(struct cafebabe_code_attribute_exception *eh,
	unsigned long offset)
{
	return eh->start_pc <= offset && offset < eh->end_pc;
}

#endif
