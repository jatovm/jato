#ifndef JIT_SUBROUTINE_H
#define JIT_SUBROUTINE_H

struct parse_context;
struct vm_method;

int inline_subroutines(struct vm_method *method);
int fail_subroutine_bc(struct parse_context *ctx);

#define convert_jsr	fail_subroutine_bc
#define convert_jsr_w	fail_subroutine_bc
#define convert_ret	fail_subroutine_bc

#endif /* JIT_SUBROUTINE_H */
