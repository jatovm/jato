#ifndef __JIT_BYTECODES_H
#define __JIT_BYTECODES_H

#include <stdbool.h>

unsigned long bytecode_size(unsigned char *);
bool bytecode_is_branch(unsigned char);
bool can_fall_through(unsigned char);
unsigned long bytecode_br_target(unsigned char *);

#endif
