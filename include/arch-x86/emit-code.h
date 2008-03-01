#ifndef __JIT_X86_OBJCODE_H
#define __JIT_X86_OBJCODE_H

#include <arch/instruction.h>

struct buffer;
struct basic_block;
struct compilation_unit;

void emit_prolog(struct buffer *, unsigned long);
void emit_epilog(struct buffer *, unsigned long);
void emit_body(struct basic_block *, struct buffer *);
void emit_trampoline(struct compilation_unit *, void *, struct buffer *);
    
#endif
