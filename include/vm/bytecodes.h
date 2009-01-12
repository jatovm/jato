#ifndef __JIT_BYTECODES_H
#define __JIT_BYTECODES_H

#include <stdbool.h>

unsigned long bc_insn_size(unsigned char *);
bool bc_is_branch(unsigned char);
bool bc_is_goto(unsigned char);
bool bc_is_wide(unsigned char);
long bc_target_off(unsigned char *);

#endif
