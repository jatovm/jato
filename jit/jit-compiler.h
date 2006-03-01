#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <compilation-unit.h>
#include <basic-block.h>
#include <stack.h>

struct compilation_unit;

void build_cfg(struct compilation_unit *);
int convert_to_ir(struct compilation_unit *);
int jit_compile(struct compilation_unit *);
void jit_magic_trampoline(struct compilation_unit *);

#endif
