/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__STREAM_H
#define CAFEBABE__STREAM_H

#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "cafebabe/error.h"

struct cafebabe_stream {
	char *filename;
	int fd;
	struct stat stat;

	void *virtual;
	unsigned int virtual_i;
	unsigned int virtual_n;

	enum cafebabe_errno cafebabe_errno;

	/* Only used if cafebabe_errno == CAFEBABE_ERROR_ERRNO */
	int syscall_errno;
};

int cafebabe_stream_open(struct cafebabe_stream *s, const char *filename);
void cafebabe_stream_close(struct cafebabe_stream *s);

void cafebabe_stream_open_buffer(struct cafebabe_stream *s,
	uint8_t *buf, unsigned int size);
void cafebabe_stream_close_buffer(struct cafebabe_stream *s);

const char *cafebabe_stream_error(struct cafebabe_stream *s);
int cafebabe_stream_eof(struct cafebabe_stream *s);

int cafebabe_stream_read_uint8(struct cafebabe_stream *s, uint8_t *r);
int cafebabe_stream_read_uint16(struct cafebabe_stream *s, uint16_t *r);
int cafebabe_stream_read_uint32(struct cafebabe_stream *s, uint32_t *r);

uint8_t *cafebabe_stream_pointer(struct cafebabe_stream *s);
int cafebabe_stream_skip(struct cafebabe_stream *s, unsigned int n);

void *cafebabe_stream_malloc(struct cafebabe_stream *s, size_t size);

#endif
