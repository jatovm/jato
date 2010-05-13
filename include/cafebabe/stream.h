/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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
