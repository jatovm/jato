#pragma once

#ifndef CAFEBABE__ATTRIBUTE_INFO_H
#define CAFEBABE__ATTRIBUTE_INFO_H

#include <stdint.h>

struct cafebabe_stream;

struct cafebabe_attribute_info {
};

static inline int
cafebabe_attribute_info_init(struct cafebabe_attribute_info *ai,
	struct cafebabe_stream *s)
{
	return 0;
}

static inline void
cafebabe_attribute_info_deinit(struct cafebabe_attribute_info *ai)
{
}

#endif
