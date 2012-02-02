/*
 * Copyright (c) 2012  Pekka Enberg
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

#include "vm/backtrace.h"

#include <execinfo.h>
#include <stdbool.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bfd.h>

#include "vm/stack-trace.h"
#include "vm/method.h"
#include "vm/class.h"
#include "vm/trace.h"
#include "vm/die.h"

#include "lib/string.h"
#include "lib/symbol.h"

#include <ucontext.h>

static unsigned long code_bytes = 64;

static void show_code(void *eip)
{
	unsigned long code_prologue = code_bytes * 43 / 64;
	unsigned char *code;
	unsigned int i;

	if (!eip)
		return;

	code	= eip - code_prologue;

	trace_printf("Code: ");
	for (i = 0; i < code_bytes; i++) {
		if (code+i == eip)
			trace_printf("<%02x> ", code[i]);
		else
			trace_printf("%02x ", code[i]);
	}
	trace_printf("\n");
}

void show_function(void *addr)
{
	char buf[128];
	char *sym;

	sym = symbol_lookup((unsigned long) addr, buf, ARRAY_SIZE(buf));

	if (sym)
		trace_printf("%s\n", sym);
	else
		trace_printf("<unknown>\n");
}

#define STACK_ENTRIES_PER_LINE		4
#define STACK_DUMP_DEPTH		(3 * STACK_ENTRIES_PER_LINE)

#define IP_REG_NAME "PC"

static void show_stack(unsigned long *sp)
{
	unsigned int i;

	trace_printf("Stack:\n");

	for (i = 0; i < STACK_DUMP_DEPTH; i++) {
		if (i && (i % STACK_ENTRIES_PER_LINE) == 0)
			trace_printf("\n");

		trace_printf("%08lx ", sp[i]);
	}

	trace_printf("\n");
}

static void show_registers(struct sigcontext *sc)
{
	unsigned long fp, sp, pc;
	unsigned long r0, r1, r2, r3;
	unsigned long r4, r5, r6, r7;
	unsigned long r8, r9, r10;

	fp	= sc->arm_fp;
	sp	= sc->arm_sp;
	pc	= sc->arm_pc;

	r0	= sc->arm_r0;
	r1	= sc->arm_r1;
	r2	= sc->arm_r2;
	r3	= sc->arm_r3;

	r4	= sc->arm_r4;
	r5	= sc->arm_r5;
	r6	= sc->arm_r6;
	r7	= sc->arm_r7;

	r8	= sc->arm_r8;
	r9	= sc->arm_r9;
	r10	= sc->arm_r10;

	trace_printf("Registers:\n");
	trace_printf(" fp:  %08lx   sp:  %08lx   pc:  %08lx\n", fp, sp, pc);
	trace_printf(" r0:  %08lx   r1:  %08lx   r2:  %08lx   r3: %08lx\n", r0, r1, r2, r3);
	trace_printf(" r4:  %08lx   r5:  %08lx   r6:  %08lx   r7: %08lx\n", r4, r5, r6, r7);
	trace_printf(" r8:  %08lx   r9:  %08lx   r10: %08lx\n", r8, r9, r10);
}

void print_backtrace_and_die(int sig, siginfo_t *info, void *secret)
{
	ucontext_t *uc = secret;
        struct sigcontext *sc = &uc->uc_mcontext;

	unsigned long ip, bp, sp, addr;

	ip	= sc->arm_pc;
	bp	= sc->arm_fp;
	sp	= sc->arm_sp;
	addr	= (unsigned long) info->si_addr;

	switch (sig) {
	case SIGSEGV:
		trace_printf("SIGSEGV at %s %08lx while accessing memory address %08lx.\n",
			IP_REG_NAME, ip, addr);
		break;
	case SIGILL:
		trace_printf("SIGILL at %s %08lx\n", sig, IP_REG_NAME, ip);
		break;
	default:
		trace_printf("Signal %d at %s %08lx\n", sig, IP_REG_NAME, ip);
		break;
	};

	show_registers(sc);

	show_stack((void *) sp);

	show_code((void *) ip);

	print_trace_from(ip, (void *) bp);

	trace_flush();

	abort();
}
