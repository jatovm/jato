/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/jit.h>
#include <vm/vm.h>
#include <compilation-unit.h>
#include <jit-compiler.h>

typedef void (*java_main_fn)(void);

void *jit_prepare_for_exec(struct methodblock *mb)
{
	mb->compilation_unit = alloc_compilation_unit(mb->code, mb->code_size);
	mb->compilation_unit->cb = CLASS_CB(mb->class);
	mb->trampoline = build_jit_trampoline(mb->compilation_unit);

	return mb->trampoline->objcode;
}
