#ifndef __JIT_BYTECODES_H
#define __JIT_BYTECODES_H

#include <stdbool.h>

unsigned long bc_insn_size(unsigned char *);
bool bc_is_branch(unsigned char);
bool bc_is_goto(unsigned char);
bool bc_is_wide(unsigned char);
long bc_target_off(unsigned char *);
bool bc_is_athrow(unsigned char);
bool bc_is_return(unsigned char);

static inline bool bc_branches_to_follower(unsigned char code)
{
	return bc_is_branch(code) && !bc_is_goto(code);
}

#endif
