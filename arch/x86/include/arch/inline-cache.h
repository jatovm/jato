#ifndef INLINE_CACHE_H
#define INLINE_CACHE_H

#include "arch/registers.h"

#include <stdbool.h>

#define IC_IMM_REG	MACH_REG_xAX
#define IC_CLASS_REG	MACH_REG_xCX

struct vm_class;
struct vm_method;
struct compilation_unit;

void *ic_lookup_vtable(struct vm_class *vmc, struct vm_method *vmm);
bool ic_supports_method(struct vm_method *vmm);
void *do_ic_setup(struct vm_class *vmc, struct vm_method *i_vmm, void *callsite);
int convert_ic_calls(struct compilation_unit *cu);
void *resolve_ic_miss(struct vm_class *vmc, struct vm_method *vmm, void *callsite);

void ic_start(void);
void ic_vcall_stub(void);

#endif /* INLINE_CACHE_H */
