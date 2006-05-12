#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <compilation-unit.h>
#include <basic-block.h>
#include <vm/stack.h>

struct methodblock;
struct compilation_unit;

struct jit_trampoline {
	void *objcode;
};

int build_cfg(struct compilation_unit *);
int convert_to_ir(struct compilation_unit *);
int jit_compile(struct compilation_unit *);
void *jit_magic_trampoline(struct compilation_unit *);

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *);
void free_jit_trampoline(struct jit_trampoline *);

void *jit_prepare_for_exec(struct methodblock *);

#endif
