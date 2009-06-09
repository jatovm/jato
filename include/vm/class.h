#ifndef __CLASS_H
#define __CLASS_H

#include <vm/types.h>
#include <vm/vm.h>

struct object *new_exception(char *class_name, char *message);
unsigned long is_object_instance_of(struct object *obj, struct object *type);
void check_array(struct object *obj, unsigned int index);
void check_cast(struct object *obj, struct object *type);
void array_store_check(struct object *arrayref, struct object *obj);
void array_store_check_vmtype(struct object *arrayref, enum vm_type vm_type);

#endif /* __CLASS_H */
