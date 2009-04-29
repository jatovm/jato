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

#include <vm/backtrace.h>
#include <stddef.h>

static void signal_handler(int sig, siginfo_t *si, void *unused)
{
	print_backtrace_and_die(sig, si, unused);
}

void setup_signal_handlers(void)
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags	= SA_RESTART | SA_SIGINFO;
	sa.sa_sigaction	= signal_handler;
   
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
}
