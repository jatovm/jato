#ifndef __VM_ALLOC_H
#define __VM_ALLOC_H

#include <stddef.h>

struct buffer;

void *alloc_exec(size_t);
int expand_buffer_exec(struct buffer *);

#endif
