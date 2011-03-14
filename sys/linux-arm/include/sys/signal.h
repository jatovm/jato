#ifndef SYS_LINUX_ARM__SIGNAL_H
#define SYS_LINUX_ARM__SIGNAL_H

#include "arch/registers.h"

#include <ucontext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

static inline void
save_signal_registers(struct register_state *regs, mcontext_t *mcontext)
{
	assert(!"not implemented");
}

struct compilation_unit;

bool signal_from_native(void *ctx);
struct compilation_unit *get_signal_source_cu(void *ctx);

#endif /* SYS_LINUX_ARM__SIGNAL_H */
