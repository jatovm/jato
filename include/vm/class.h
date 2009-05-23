#ifndef __CLASS_H
#define __CLASS_H

#include <vm/vm.h>

unsigned long is_object_instance_of(struct object *obj, struct object *type);
void check_null(struct object *obj);
void check_array(struct object *obj, unsigned int index);
void check_cast(struct object *obj, struct object *type);

#include <vm/field.h>
#include <vm/method.h>

#include <jit/vtable.h>

struct vm_class {
	const struct cafebabe_class *class;

	char *name;

	struct vm_class *super;
	struct vm_field *fields;
	struct vm_method *methods;

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

int vm_class_resolve_field(struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_field *vm_class_resolve_field_recursive(struct vm_class *vmc,
	uint16_t i);

int vm_class_resolve_method(struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type);
struct vm_method *vm_class_resolve_method_recursive(struct vm_class *vmc,
	uint16_t i);

#endif /* __CLASS_H */
