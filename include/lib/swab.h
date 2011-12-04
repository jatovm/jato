#ifndef JATO__LIB_SWAB_H
#define JATO__LIB_SWAB_H

#include <stdint.h>

static inline uint16_t swab16(uint16_t x)
{
	return (x & 0xff00U >> 8) | (x & 0x00ffU << 8);
}

static inline uint32_t swab32(uint32_t x)
{
	return __builtin_bswap32(x);
}

static inline uint64_t swab64(uint64_t x)
{
	return __builtin_bswap64(x);
}

#endif /* JATO__LIB_SWAB_H */
