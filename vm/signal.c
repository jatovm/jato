/*
 * Copyright (C) 2009 Pekka Enberg
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

#include <jit/exception.h>

#include <vm/backtrace.h>
#include <vm/signal.h>
#include <vm/class.h>

#include <arch/signal.h>

#include <ucontext.h>
#include <stddef.h>
#include <unistd.h>

static void throw_arithmetic_exception(void)
{
	signal_new_exception("java/lang/ArithmeticException",
			     "division by zero");
	throw_from_native(0);
}

static void throw_null_pointer_exception(void)
{
	signal_new_exception("java/lang/NullPointerException", NULL);
	throw_from_native(0);
}

static void sigfpe_handler(int sig, siginfo_t *si, void *ctx)
{
	if (signal_from_jit_method(ctx) && si->si_code == FPE_INTDIV) {
		if (install_signal_bh(ctx, throw_arithmetic_exception) == 0)
			return;

		fprintf(stderr, "%s: install_signal_bh() failed.\n", __func__);
	}

	print_backtrace_and_die(sig, si, ctx);
}

static void sigsegv_handler(int sig, siginfo_t *si, void *ctx)
{
	if (!signal_from_jit_method(ctx))
		goto exit;

	/* Assume that zero-page access is caused by dereferencing a
	   null pointer */
	if ((unsigned long)si->si_addr < (unsigned long)getpagesize()) {
		/* We must be extra caucious here because IP might be
		   invalid */
		if (get_signal_source_cu(ctx) == NULL)
			goto exit;

		if (install_signal_bh(ctx, throw_null_pointer_exception) == 0)
			return;

		fprintf(stderr, "%s: install_signal_bh() failed.\n", __func__);
		goto exit;
	}

	/* Check if exception was triggered by exception guard */
	if (si->si_addr == exceptions_guard_page ||
	    si->si_addr == trampoline_exceptions_guard_page) {
		struct object *exception;

		exception = exception_occurred();
		if (exception == NULL) {
			fprintf(stderr, "%s: spurious exception-test failure\n",
				__func__);
			goto exit;
		}

		if (si->si_addr == trampoline_exceptions_guard_page)
			throw_exception_from_trampoline(ctx, exception);
		else
			throw_exception_from_signal(ctx, exception);

		return;
	}

 exit:
	print_backtrace_and_die(sig, si, ctx);
}

static void signal_handler(int sig, siginfo_t *si, void *ctx)
{
	print_backtrace_and_die(sig, si, ctx);
}

void setup_signal_handlers(void)
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags	= SA_RESTART | SA_SIGINFO;

	sa.sa_sigaction	= sigsegv_handler;
	sigaction(SIGSEGV, &sa, NULL);

	sa.sa_sigaction	= sigfpe_handler;
	sigaction(SIGFPE, &sa, NULL);

	sa.sa_sigaction	= signal_handler;
	sigaction(SIGUSR1, &sa, NULL);
}
