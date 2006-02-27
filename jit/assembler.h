#ifndef __JIT_ASSEMBLER_H
#define __JIT_ASSEMBLER_H

struct basic_block;

void assemble(struct basic_block *, unsigned char *, unsigned long);

#endif
