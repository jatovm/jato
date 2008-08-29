#include <vm/vm.h>
#include <test/vm.h>

static void *resolve(struct object *class, int cp_index)
{
	struct constant_pool *cp = &(CLASS_CB(class)->constant_pool);
	return (void *) CP_INFO(cp, cp_index);
}

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

struct object *resolveClass(struct object *class, int cp_index, int init)
{
	return resolve(class, cp_index);
}

struct object *findArrayClassFromClassLoader(char *name, struct object *classloader)
{
	return new_class();
}
