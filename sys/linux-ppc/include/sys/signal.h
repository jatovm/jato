#ifndef JATO__SYS_LINUX_PPC_SIGNAL_H
#define JATO__SYS_LINUX_PPC_SIGNAL_H

#include "arch/registers.h"

#include <ucontext.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

struct compilation_unit;

static inline void save_signal_registers(struct register_state *regs, union uc_regs_ptr *uc_regs)
{
	assert(!"not implemented");
}

bool signal_from_native(void *ctx);

struct compilation_unit *get_signal_source_cu(void *ctx);

#endif /* JATO__SYS_LINUX_PPC_SIGNAL_H */
