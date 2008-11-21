#ifndef VM_LIMITS_H
#define VM_LIMITS_H

#include <stdint.h>

/*
 * The values of the integral types of the Java virtual machine are the same as
 * those for the integral types of the Java programming language (§2.4.1):
 */

/* For byte, from -128 to 127 (-2^7 to 2^7-1), inclusive */
#define J_BYTE_MIN ((int8_t)-(1ULL << 7)     )
#define J_BYTE_MAX ((int8_t)((1ULL << 7) - 1))

/* For short, from -32768 to 32767 (-2^15 to 2^15-1), inclusive */
#define J_SHORT_MIN ((int16_t)-(1ULL << 15)     )
#define J_SHORT_MAX ((int16_t)((1ULL << 15) - 1))

/* For int, from -2147483648 to 2147483647 (-2^31 to 2^31-1), inclusive */
#define J_INT_MIN ((int32_t)-(1ULL << 31)     )
#define J_INT_MAX ((int32_t)((1ULL << 31) - 1))

/* For long, from -9223372036854775808 to 9223372036854775807 (-2^63 to 2^63-1), inclusive */
#define J_LONG_MIN ((int64_t)-(1ULL << 63)     )
#define J_LONG_MAX ((int64_t)((1ULL << 63) - 1))

#endif /* VM_LIMITS_H */
