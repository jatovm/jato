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
struct buffer *alloc_buffer(size_t);
void free_buffer(struct buffer *);
int append_buffer(struct buffer *buf, unsigned char);

static inline void *buffer_ptr(struct buffer *buf)
{
	return buf->buf;
}

static inline void *buffer_current(struct buffer *buf)
{
	return (void *) buf->buf + buf->offset;
}

static inline size_t buffer_offset(struct buffer *buf)
{
	return buf->offset;
}

void generic_buffer_free(struct buffer *);

#endif
