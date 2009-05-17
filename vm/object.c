#include <stdlib.h>

#include "vm/object.h"

#define NOT_IMPLEMENTED \
	fprintf(stderr, "%s:%d: warning: %s not implemented\n",	\
		__FILE__, __LINE__, __func__)

struct vm_object *vm_object_alloc(struct vm_class *class)
{
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_object *vm_object_alloc_native_array(int type, int count)
{
	NOT_IMPLEMENTED;
	return NULL;
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
