#ifndef __JIT_SYSTEM_H
#define __JIT_SYSTEM_H

#include <stddef.h>

#define BITS_PER_LONG (sizeof(unsigned long) * 8)

/* Macros stolen shamelessly from Linux kernel. */
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN(x,a) (((x)+(a)-1)&~((a)-1))

#endif
