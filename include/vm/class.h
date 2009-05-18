#ifndef __CLASS_H
#define __CLASS_H

#include <vm/vm.h>

unsigned long is_object_instance_of(struct object *obj, struct object *type);
void check_null(struct object *obj);
void check_array(struct object *obj, unsigned int index);
void check_cast(struct object *obj, struct object *type);

#include <vm/method.h>

struct vm_class {
	const struct cafebabe_class *class;

	char *name;

	struct vm_method *methods;
};

int vm_class_init(struct vm_class *vmc, const struct cafebabe_class *class);

struct vm_method *vm_class_resolve_method(struct vm_class *vmc, uint16_t i);

#endif /* __CLASS_H */
