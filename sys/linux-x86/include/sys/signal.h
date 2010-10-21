#ifndef _ARCH_SIGNAL_H
#define _ARCH_SIGNAL_H

#include <stdbool.h>
#include <stdlib.h>
#include <ucontext.h>

#ifdef CONFIG_X86_32
# include "signal_32.h"
#else
# include "signal_64.h"
#endif

struct compilation_unit;

bool signal_from_native(void *ctx);
struct compilation_unit *get_signal_source_cu(void *ctx);

#endif
