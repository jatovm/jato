#ifndef JATO_EMIT_CODE_H
#define JATO_EMIT_CODE_H

struct compilation_unit;
struct jit_trampoline;
struct basic_block;
struct buffer;
struct vm_object;

void emit_prolog(struct buffer *, unsigned long);
void emit_epilog(struct buffer *);
void emit_body(struct basic_block *, struct buffer *);
void emit_trampoline(struct compilation_unit *, void *, struct jit_trampoline *);
void emit_unwind(struct buffer *);
void emit_lock(struct buffer *, struct vm_object *);
void emit_lock_this(struct buffer *);
void emit_unlock(struct buffer *, struct vm_object *);
void emit_unlock_this(struct buffer *);

#endif /* JATO_EMIT_CODE_H */
