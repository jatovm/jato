/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/buffer.h>
#include <stdlib.h>
#include <string.h>

struct buffer *__alloc_buffer(size_t size, struct buffer_operations *ops)
{
	struct buffer *buf;
	int err;
       
	buf = malloc(sizeof *buf);
	if (!buf)
		return NULL;

	memset(buf, 0, sizeof *buf);

	buf->ops = ops;

	err = ops->expand(buf, size);
	if (err)
		return NULL;

	return buf;
}

void free_buffer(struct buffer *buf)
{
	buf->ops->free(buf);
	free(buf);
}

int append_buffer(struct buffer *buf, unsigned char c)
{
	if (buf->offset == buf->size) {
		int err;
		
		err = buf->ops->expand(buf, buf->size + 1);
		if (err)
			return err;
	}
	buf->buf[buf->offset++] = c;
	return 0;
}
