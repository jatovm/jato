#ifndef JATO__LIB_LITTLE_ENDIAN_H
#define JATO__LIB_LITTLE_ENDIAN_H

#include "lib/swab.h"

static inline uint16_t le16_to_cpu(uint16_t x) { return x; }
static inline uint32_t le32_to_cpu(uint32_t x) { return x; }
static inline uint64_t le64_to_cpu(uint64_t x) { return x; }
static inline uint16_t be16_to_cpu(uint16_t x) { return swab16(x); }
static inline uint32_t be32_to_cpu(uint32_t x) { return swab32(x); }
static inline uint64_t be64_to_cpu(uint64_t x) { return swab64(x); }

#endif /* JATO__LIB_LITTLE_ENDIAN_H */
