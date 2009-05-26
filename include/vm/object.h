#ifndef __VM_OBJECT_H
#define __VM_OBJECT_H

#include <vm/class.h>
#include <vm/vm.h>

struct vm_object {
	struct vm_class *class;

	unsigned int array_length;

	uint8_t fields[];
};

struct vm_object *vm_object_alloc(struct vm_class *class);
struct vm_object *vm_object_alloc_native_array(int type, int count);
struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int **count);
struct vm_object *vm_object_alloc_array(struct vm_class *class, int count);

struct vm_object *
vm_object_alloc_string(const uint8_t bytes[], unsigned int length);

void vm_object_lock(struct vm_object *obj);
void vm_object_unlock(struct vm_object *obj);

#endif
