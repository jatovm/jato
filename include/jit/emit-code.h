#ifndef JATO_EMIT_CODE_H
#define JATO_EMIT_CODE_H

struct compilation_unit;
struct jit_trampoline;
struct basic_block;
struct buffer;
struct insn;
struct vm_object;
struct vm_jni_env;
struct vm_method;

extern void emit_prolog(struct buffer *, unsigned long);
extern void emit_trace_invoke(struct buffer *, struct compilation_unit *);
extern void emit_epilog(struct buffer *);
extern void emit_trampoline(struct compilation_unit *, void *, struct jit_trampoline *);
extern void emit_unwind(struct buffer *);
extern void emit_lock(struct buffer *, struct vm_object *);
extern void emit_lock_this(struct buffer *);
extern void emit_unlock(struct buffer *, struct vm_object *);
extern void emit_unlock_this(struct buffer *);
extern void emit_body(struct basic_block *, struct buffer *);
extern void emit_insn(struct buffer *, struct basic_block *, struct insn *);
extern void emit_nop(struct buffer *buf);
extern void backpatch_branch_target(struct buffer *buf, struct insn *insn,
				    unsigned long target_offset);
extern void emit_jni_trampoline(struct buffer *, struct vm_method *, void *);

extern void *emit_ic_check(struct buffer *);
extern void emit_ic_miss_handler(struct buffer *, void *, struct vm_method *);

#endif /* JATO_EMIT_CODE_H */
