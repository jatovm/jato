#ifndef JATO__PPC_ITABLE_H
#define JATO__PPC_ITABLE_H

struct vm_method;
struct vm_object;

void itable_resolver_stub_error(struct vm_method *method, struct vm_object *obj);

#endif
