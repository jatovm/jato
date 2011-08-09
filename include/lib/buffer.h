#ifndef __BUFFER_H
#define __BUFFER_H

#include <stddef.h>
#include <stdint.h>

struct buffer;

/**
 *	@expand: This method is called when buffer needs to be expanded to
 *		fit one more element. Returns zero on success; otherwise
 *		returns a negative number.
 *	@free: This method is called when the whole buffer is being discarded.
 */
struct buffer_operations {
	int (*expand)(struct buffer *);
	void (*free)(struct buffer *);
};

struct buffer {
	unsigned char *buf;
	size_t offset;
	size_t size;
	struct buffer_operations *ops;
};

struct buffer *alloc_exec_buffer(void);
struct buffer *__alloc_buffer(struct buffer_operations *);
struct buffer *alloc_buffer(void);
void free_buffer(struct buffer *);
int append_buffer_str(struct buffer *buf, unsigned char *str, size_t len);

static inline int append_buffer(struct buffer *buf, unsigned char c)
{
	if (buf->size - buf->offset < 1)
		return append_buffer_str(buf, &c, 1);

	buf->buf[buf->offset++] = c;

	return 0;
}

static inline void buffer_write8(struct buffer *b, uint8_t x)
{
	append_buffer(b, x);
}

static inline void buffer_write_be32(struct buffer *b, uint32_t x)
{
	buffer_write8(b, (x >> 24) & 0xff);
	buffer_write8(b, (x >> 16) & 0xff);
	buffer_write8(b, (x >>  8) & 0xff);
	buffer_write8(b, (x      ) & 0xff);
}

static inline uint8_t buffer_read8(struct buffer *b)
{
	return b->buf[b->offset++];
}

static inline uint32_t buffer_read_be32(struct buffer *b)
{
	return buffer_read8(b) << 24 | buffer_read8(b) << 16
			| buffer_read8(b) << 8 | buffer_read8(b);
}

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

static inline void buffer_flip(struct buffer *buf)
{
	buf->offset = 0;
}

void generic_buffer_free(struct buffer *);

#endif
