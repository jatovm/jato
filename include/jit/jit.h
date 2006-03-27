#ifndef __JIT_H
#define __JIT_H

struct methodblock;

void *jit_prepare_for_exec(struct methodblock *);

#endif
