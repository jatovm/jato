/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

#ifndef CAFEBABE__STREAM_H
#define CAFEBABE__STREAM_H

#include <sys/stat.h>
#include <sys/types.h>

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

void *cafebabe_stream_malloc(struct cafebabe_stream *s, size_t size);

#endif
