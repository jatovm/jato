/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/buffer.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct buffer *__alloc_buffer(struct buffer_operations *ops)
{
	struct buffer *buf;
       
	buf = malloc(sizeof *buf);
	if (!buf)
		return NULL;

	memset(buf, 0, sizeof *buf);

	buf->ops = ops;

	return buf;
}

void free_buffer(struct buffer *buf)
{
	if (buf)
		buf->ops->free(buf);

	free(buf);
}

int append_buffer(struct buffer *buf, unsigned char c)
{
	if (buf->offset == buf->size) {
		int err;
		
		err = buf->ops->expand(buf);
		if (err)
			return err;
	}
	buf->buf[buf->offset++] = c;
	return 0;
}

static int generic_buffer_expand(struct buffer *buf)
{
	size_t size;
	void *p;

	size = buf->size + 1;
	p = realloc(buf->buf, size);
	if (!p)
		return -ENOMEM;

	buf->buf  = p;
	buf->size = size;
	return 0;
}

void generic_buffer_free(struct buffer *buf)
{
	free(buf->buf);
}

static struct buffer_operations generic_buffer_ops = {
	.expand = generic_buffer_expand,
	.free   = generic_buffer_free,
};

struct buffer *alloc_buffer(void)
{
	return __alloc_buffer(&generic_buffer_ops);
}
