#pragma once

#ifndef CAFEBABE__FIELD_INFO_H
#define CAFEBABE__FIELD_INFO_H

struct cafebabe_stream;

struct cafebabe_field_info {
};

static inline int
cafebabe_field_info_init(struct cafebabe_field_info *fi,
	struct cafebabe_stream *s)
{
	return 0;
}

static inline void
cafebabe_field_info_deinit(struct cafebabe_field_info *fi)
{
}

#endif
