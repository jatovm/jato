#ifndef __VM_FIELD_H
#define __VM_FIELD_H

#include <vm/vm.h>
#include <vm/types.h>

static inline enum vm_type field_type(struct fieldblock *fb)
{
	return str_to_type(fb->type);
}

#endif
