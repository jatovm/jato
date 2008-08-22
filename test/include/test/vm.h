#ifndef __VM_UTILS_H
#define __VM_UTILS_H

#include <vm/vm.h>
#include <stdlib.h>

static inline struct object *new_class(void)
{
	return malloc(sizeof(struct classblock) + sizeof(struct object));
}

#define RESOLVED_STRING_CONSTANT 0x2bad1deaUL

#endif
