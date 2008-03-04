#ifndef __PPC_EMIT_CODE_H
#define __PPC_EMIT_CODE_H

#include <arch/instruction.h>

struct buffer;
struct basic_block;
struct compilation_unit;

static inline void emit_prolog(struct buffer *buf, unsigned long nr)
{
}
    
static inline void emit_epilog(struct buffer *buf, unsigned long nr)
{
}
    
static inline void emit_body(struct basic_block *bb, struct buffer *buf)
{
}
    
static inline void emit_trampoline(struct compilation_unit *cu, void *p, struct buffer *buf)
{
}
    
#endif /* __PPC_EMIT_CODE_H */
