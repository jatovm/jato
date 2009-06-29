#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <vm/class.h>
#include <vm/object.h>

struct vm_object *
vm_object_alloc_string(const uint8_t bytes[], unsigned int length)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_object *new_exception(const char *class_name, const char *message)
{
	return NULL;
}

bool vm_object_is_instance_of(const struct vm_object *obj,
	const struct vm_class *type)
{
	if (!obj)
		return 0;

	return obj->class == type;
}

void vm_object_check_array(struct vm_object *obj, unsigned int index)
{
	struct vm_class *cb = obj->class;

	if (!IS_ARRAY(cb))
		abort();

	if (index >= ARRAY_LEN(obj))
		abort();
}

void check_cast(struct vm_object *obj, struct vm_class *type)
{
	if (!obj)
		return;

	if (!vm_object_is_instance_of(obj, type))
		abort();
}

void array_store_check(struct vm_object *arrayref, struct vm_object *obj)
{
}

void array_store_check_vmtype(struct vm_object *arrayref, enum vm_type vm_type)
{
}

void array_size_check(int size)
{
}

void multiarray_size_check(int n, ...)
{
}
