#ifndef __VM_METHOD_H
#define __VM_METHOD_H

#include <vm/vm.h>
#include <vm/types.h>

#include <stdbool.h>
#include <string.h>

static inline enum vm_type method_return_type(struct methodblock *method)
{
	char *return_type = method->type + (strlen(method->type) - 1);
	return str_to_type(return_type);
}

static inline bool method_is_constructor(struct methodblock *method)
{
	return strcmp(method->name,"<init>") == 0;
}

static inline bool method_is_synchronized(struct methodblock *method)
{
	return method->access_flags & ACC_SYNCHRONIZED;
}

static inline bool method_is_static(struct methodblock *method)
{
	return method->access_flags & ACC_STATIC;
}

#endif
