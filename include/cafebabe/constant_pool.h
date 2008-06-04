#pragma once

#ifndef CAFEBABE__CONSTANT_POOL_H
#define CAFEBABE__CONSTANT_POOL_H

#include <stdint.h>

struct cafebabe_stream;

struct cafebabe_constant_pool {
	uint8_t tag;
	uint8_t *info;
};

static inline int
cafebabe_constant_pool_init(struct cafebabe_constant_pool *cp,
	struct cafebabe_stream *s)
{
	return 0;
}

static inline void
cafebabe_constant_pool_deinit(struct cafebabe_constant_pool *cp)
{
}

#endif
