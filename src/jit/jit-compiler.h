#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <compilation-unit.h>
#include <basic-block.h>
#include <stack.h>

extern unsigned char bytecode_sizes[];

void build_cfg(struct compilation_unit *);
int convert_to_ir(struct compilation_unit *);

#endif
