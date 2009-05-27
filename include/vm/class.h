#ifndef __CLASS_H
#define __CLASS_H

#include <vm/field.h>
#include <vm/method.h>
#include <vm/vm.h>

#include <jit/vtable.h>

enum vm_class_state {
	VM_CLASS_LOADED,
	VM_CLASS_LINKED,
	VM_CLASS_INITIALIZED,
};

struct vm_class {
	const struct cafebabe_class *class;

	enum vm_class_state state;

	char *name;

	struct vm_class *super;
	struct vm_field *fields;
	struct vm_method *methods;

	unsigned int object_size;

	unsigned int vtable_size;
	struct vtable vtable;
};

int vm_class_init(struct vm_class *vmc, const struct cafebabe_class *class);

static inline bool vm_class_is_interface(struct vm_class *vmc)
{
	return vmc->class->access_flags & CAFEBABE_CLASS_ACC_INTERFACE;
}

int vm_class_run_clinit(struct vm_class *vmc);

struct vm_class *vm_class_resolve_class(struct vm_class *vmc, uint16_t i);

struct vm_field *vm_class_get_field(struct vm_class *vmc,
	const char *name, const char *type);
struct vm_field *vm_class_get_field_recursive(struct vm_class *vmc,
	const char *name, const char *type);

int vm_class_resolve_field(struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_field *vm_class_resolve_field_recursive(struct vm_class *vmc,
	uint16_t i);

struct vm_method *vm_class_get_method(struct vm_class *vmc,
	const char *name, const char *type);
struct vm_method *vm_class_get_method_recursive(struct vm_class *vmc,
	const char *name, const char *type);

int vm_class_resolve_method(struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_method *vm_class_resolve_method_recursive(struct vm_class *vmc,
	uint16_t i);

#endif /* __CLASS_H */
