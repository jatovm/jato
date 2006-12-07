#ifndef __VM_STREAM_H
#define __VM_STREAM_H

#include <stdbool.h>

struct stream;

struct stream_operations {
	unsigned char *(*new_position)(struct stream *stream);
};

struct stream {
	unsigned char *start;
	unsigned char *end;
	unsigned char *current;
	struct stream_operations *ops;
};

static inline void stream_init(struct stream *stream,
			unsigned char *start,
			unsigned long len,
			struct stream_operations *ops)
{
	stream->start   = start;
	stream->end     = start + len;
	stream->current = start;
	stream->ops     = ops;
}

static inline bool stream_has_more(struct stream *stream)
{
	return stream->current != stream->end;
}

static inline void stream_advance(struct stream *stream)
{
	stream->current = stream->ops->new_position(stream);
}

static inline unsigned long stream_offset(struct stream *stream)
{
	return stream->current - stream->start;
}

#endif
