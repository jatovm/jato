#ifndef __JIT_VTABLE_H
#define __JIT_VTABLE_H

struct compilation_unit;

struct vtable {
	void **native_ptr;
};

void vtable_init(struct vtable *vtable, unsigned int nr_methods);
void vtable_release(struct vtable *vtable);
void vtable_setup_method(struct vtable *vtable, unsigned long idx, void *native_ptr);
void fixup_vtable(struct compilation_unit *cu, void *target);

#endif /* __JIT_VTABLE_H */
