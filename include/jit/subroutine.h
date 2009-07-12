#ifndef JATO_JIT_SUBROUTINE
#define JATO_JIT_SUBROUTINE

struct vm_method;
struct parse_context;

int inline_subroutines(struct vm_method *method);
int subroutines_should_not_occure(struct parse_context *ctx);

#define convert_jsr subroutines_should_not_occure
#define convert_jsr_w subroutines_should_not_occure
#define convert_ret subroutines_should_not_occure

#endif
