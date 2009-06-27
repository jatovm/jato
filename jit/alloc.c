/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for allocating special-purpose memory in the
 * JIT compiler. The executable memory allocation is shamelessly taken from
 * Parrot.
 */

#define _XOPEN_SOURCE 600

#include <vm/buffer.h>
#include <vm/system.h>

#include <errno.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static void *__alloc_exec(size_t size)
{
	int page_size;
	void *p;

	page_size = getpagesize();

	if (posix_memalign(&p, page_size, size))
		return NULL;
	mprotect(p, size, PROT_READ | PROT_WRITE | PROT_EXEC);

	return p;
}

int expand_buffer_exec(struct buffer *buf)
{
	int page_size;
	size_t size;
	void *p;

	page_size = getpagesize();
	size = ALIGN(buf->size + 1, page_size);

	p = __alloc_exec(size);
	if (!p)
		return -ENOMEM;
	memset(p, 0, size);

	if (buf->size)
		memcpy(p, buf->buf, buf->size);

	buf->ops->free(buf);

	buf->buf  = p;
	buf->size = size;

	return 0;
}

void *alloc_page(void)
{
	int page_size;
	void *p;

	page_size = getpagesize();

	if (posix_memalign(&p, page_size, page_size))
		return NULL;

	return p;
}
