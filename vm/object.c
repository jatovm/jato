#include <stdlib.h>

#include <vm/object.h>
#include <vm/stdlib.h>

struct vm_object *vm_object_alloc(struct vm_class *class)
{
	struct vm_object *res;

	res = zalloc(sizeof(*res) + class->object_size);
	if (res) {
		res->class = class;
	}

	return res;
}

struct vm_object *vm_object_alloc_native_array(int type, int count)
{
	struct vm_object *res;

	/* XXX: Use the right size... */
	res = zalloc(sizeof(*res) + 8 * count);
	if (res) {
		res->array_length = count;
	}

	return res;
}

struct vm_object *vm_object_alloc_multi_array(struct vm_class *class,
	int nr_dimensions, int **counts)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_object *vm_object_alloc_array(struct vm_class *class, int count)
{
	NOT_IMPLEMENTED;
	return NULL;
}

void vm_object_lock(struct vm_object *obj)
{
	NOT_IMPLEMENTED;
}

void vm_object_unlock(struct vm_object *obj)
{
	NOT_IMPLEMENTED;
}
