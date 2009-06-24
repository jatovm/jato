/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <jit/cu-mapping.h>
#include <jit/compiler.h>

#include <arch/stack-frame.h>
#include <arch/signal.h>

#include <vm/signal.h>

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
	return get_cu_from_native_addr(uc->uc_mcontext.gregs[REG_IP]);
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

	/* push return address on stack */
	stack = (unsigned long*)uc->uc_mcontext.gregs[REG_SP] - 1;
	*stack = uc->uc_mcontext.gregs[REG_IP];
	uc->uc_mcontext.gregs[REG_SP] -= sizeof(unsigned long);

	uc->uc_mcontext.gregs[REG_IP] = (unsigned long)bh;

	return 0;
}
