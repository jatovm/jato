#ifndef __JIT_ASSEMBLER_H
#define __JIT_ASSEMBLER_H

struct basic_block;

void x86_emit_obj_code(struct basic_block *, unsigned char *, unsigned long);

#endif
