#ifndef __VM_ALLOC_H
#define __VM_ALLOC_H

#include <stddef.h>

struct buffer;

void *alloc_exec(size_t);
int expand_exec(struct buffer *, size_t);

#endif
