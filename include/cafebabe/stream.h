#pragma once

#ifndef CAFEBABE__STREAM_H
#define CAFEBABE__STREAM_H

#include <sys/stat.h>
#include <sys/types.h>

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

const char *cafebabe_stream_error(struct cafebabe_stream *s);

int cafebabe_stream_read_uint8(struct cafebabe_stream *s, uint8_t *r);
int cafebabe_stream_read_uint16(struct cafebabe_stream *s, uint16_t *r);
int cafebabe_stream_read_uint32(struct cafebabe_stream *s, uint32_t *r);

#endif
