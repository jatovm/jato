#ifndef _ARCH_SIGNAL_H
#define _ARCH_SIGNAL_H

#include <stdbool.h>
#include <stdlib.h>
#include <ucontext.h>

bool signal_from_jit_method(void *ctx);

#endif
