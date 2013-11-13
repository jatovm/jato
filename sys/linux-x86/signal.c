/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "sys/signal.h"

#include "jit/cu-mapping.h"
#include "jit/compiler.h"

#include "arch/stack-frame.h"

#include "vm/signal.h"
#include "vm/trace.h"

#include <string.h>

extern void signal_bh_trampoline(void *bh);

bool signal_from_native(void *ctx)
{
	ucontext_t *uc;

	uc = ctx;

	return is_native(uc->uc_mcontext.gregs[REG_IP]);
}

struct compilation_unit *get_signal_source_cu(void *ctx)
{
	ucontext_t *uc;

	uc = ctx;
	return jit_lookup_cu(uc->uc_mcontext.gregs[REG_IP]);
}

/**
 * install_signal_bh - installs signal's bottom half function by
 *     modifying user context so that control will be returned to @bh
 *     when signal handler returns. When @bh function returns, the
 *     control should be returned to the source of the signal.
 *
 * @ctx: pointer to struct ucontext_t
 * @bh: bottom half function to install
 */
int install_signal_bh(void *ctx, signal_bh_fn bh)
{
	unsigned long *stack;
	ucontext_t *uc;

	uc = ctx;

	stack = (unsigned long *) uc->uc_mcontext.gregs[REG_SP];

	/* push bottom-half handler address on stack */
	*(--stack) = (unsigned long) bh;

	/* push return address on stack */
	*(--stack) = uc->uc_mcontext.gregs[REG_IP];

	uc->uc_mcontext.gregs[REG_SP] = (unsigned long) stack;
	uc->uc_mcontext.gregs[REG_IP] = (unsigned long) signal_bh_trampoline;

	return 0;
}

void trace_signal(int sig, siginfo_t *si, void *ctx)
{
	ucontext_t *uc = ctx;

	trace_printf("signal %2d: ip=0x%016x: %s\n", sig, (unsigned long) uc->uc_mcontext.gregs[REG_IP], strsignal(sig));
	trace_flush();
}
