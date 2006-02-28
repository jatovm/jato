#ifndef __JIT_ASSEMBLER_H
#define __JIT_ASSEMBLER_H

struct basic_block;

int x86_emit_prolog(unsigned char *, unsigned long);
int x86_emit_epilog(unsigned char *, unsigned long);
void x86_emit_obj_code(struct basic_block *, unsigned char *, unsigned long);

#endif
