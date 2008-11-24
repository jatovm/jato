#ifndef VM_LIMITS_H
#define VM_LIMITS_H

#include <stdint.h>

/*
 * The values of the integral types of the Java virtual machine are the same as
 * those for the integral types of the Java programming language (§2.4.1):
 */

/* For byte, from -128 to 127 (-2^7 to 2^7-1), inclusive */
#define J_BYTE_MIN INT8_MIN
#define J_BYTE_MAX INT8_MAX

/* For short, from -32768 to 32767 (-2^15 to 2^15-1), inclusive */
#define J_SHORT_MIN INT16_MIN
#define J_SHORT_MAX INT16_MAX

/* For int, from -2147483648 to 2147483647 (-2^31 to 2^31-1), inclusive */
#define J_INT_MIN INT32_MIN
#define J_INT_MAX INT32_MAX

/* For long, from -9223372036854775808 to 9223372036854775807 (-2^63 to 2^63-1), inclusive */
#define J_LONG_MIN INT64_MIN
#define J_LONG_MAX INT64_MAX

#endif /* VM_LIMITS_H */
