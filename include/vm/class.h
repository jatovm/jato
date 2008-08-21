#ifndef __CLASS_H
#define __CLASS_H

#include <vm/vm.h>

unsigned long is_object_instance_of(struct object *obj, struct object *type);
void check_null(struct object *obj);
void check_array(struct object *obj, unsigned int index);

#endif /* __CLASS_H */
