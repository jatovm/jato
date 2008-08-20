#ifndef __X86_EMIT_CODE_H
#define __X86_EMIT_CODE_H

struct compilation_unit;
struct basic_block;
struct buffer;

void emit_prolog(struct buffer *, unsigned long);
void emit_epilog(struct buffer *, unsigned long);
void emit_body(struct basic_block *, struct buffer *);
void emit_trampoline(struct compilation_unit *, void *, struct buffer *);
    
#endif /* __X86_EMIT_CODE */
