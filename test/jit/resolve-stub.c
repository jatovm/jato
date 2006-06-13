#include <vm/vm.h>

struct methodblock *resolveMethod(struct object *class, int cp_index)
{
	struct constant_pool *cp = &(CLASS_CB(class)->constant_pool);
	return (struct methodblock *) CP_INFO(cp, cp_index);
}

struct fieldblock *resolveField(struct object *class, int cp_index)
{
	struct constant_pool *cp = &(CLASS_CB(class)->constant_pool);
	return (struct fieldblock *) CP_INFO(cp, cp_index);
}
