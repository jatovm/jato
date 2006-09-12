#ifndef __BUFFER_H
#define __BUFFER_H

#include <stddef.h>

struct buffer;

struct buffer_operations {
	int (*expand)(struct buffer *, size_t size);
	void (*free)(struct buffer *);
};

struct buffer {
	unsigned char *buf;
	size_t offset;
	size_t size;
	struct buffer_operations *ops;
};

struct buffer *__alloc_buffer(size_t, struct buffer_operations *);
void free_buffer(struct buffer *);
int append_buffer(struct buffer *buf, unsigned char);

#endif
