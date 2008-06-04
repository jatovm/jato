#pragma once

#ifndef CAFEBABE__METHOD_INFO_H
#define CAFEBABE__METHOD_INFO_H

#include <stdint.h>

struct cafebabe_stream;

struct cafebabe_method_info {
};

static inline int
cafebabe_method_info_init(struct cafebabe_method_info *mi,
	struct cafebabe_stream *s)
{
	return 0;
}

static inline void
cafebabe_method_info_deinit(struct cafebabe_method_info *mi)
{
}

#endif
