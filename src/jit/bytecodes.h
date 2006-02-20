#ifndef __JIT_BYTECODES_H
#define __JIT_BYTECODES_H

#include <stdbool.h>

extern unsigned char bytecode_sizes[];

bool bytecode_is_branch(unsigned char);

#endif
