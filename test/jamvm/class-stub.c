#include <vm/vm.h>
#include <stdlib.h>

struct object *new_exception(char *class_name, char *message)
{
	return NULL;
}

unsigned long is_object_instance_of(struct object *obj, struct object *type)
{
	if (!obj)
		return 0;

	return isInstanceOf(type, obj->class);
}

void check_array(struct object *obj, unsigned int index)
{
	struct classblock *cb = CLASS_CB(obj->class);

	if (!IS_ARRAY(cb))
		abort();

	if (index >= ARRAY_LEN(obj))
		abort();
}

void check_cast(struct object *obj, struct object *type)
{
	if (!obj)
		return;

	if (!isInstanceOf(type, obj->class))
		abort();
}
