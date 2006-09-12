#ifndef __JIT_X86_OBJCODE_H
#define __JIT_X86_OBJCODE_H

#include <jit/instruction.h>

struct buffer;
struct basic_block;
struct compilation_unit;

void x86_emit_prolog(struct buffer *, unsigned long);
void x86_emit_epilog(struct buffer *, unsigned long);
void x86_emit_obj_code(struct basic_block *, struct buffer *);
void x86_emit_trampoline(struct compilation_unit *, void *, struct buffer *, unsigned long);
    
#endif
