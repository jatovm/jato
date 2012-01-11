/*
 * Copyright (c) 2006-2008  Pekka Enberg
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

#include <bfd.h>
#include <execinfo.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vm/die.h"
#include "vm/class.h"
#include "vm/method.h"
#include "vm/stack-trace.h"
#include "vm/trace.h"

#include "lib/string.h"
#include "lib/symbol.h"

/* get REG_EIP from ucontext.h */
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

#ifndef CONFIG_X86_64

#define IP_REG REG_EIP
#define BP_REG REG_EBP
#define SP_REG REG_ESP

#define IP_REG_NAME "EIP"

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

static void show_registers(gregset_t gregs)
{
	unsigned long eax, ebx, ecx, edx;
	unsigned long esi, edi, ebp, esp;

	eax = gregs[REG_EAX];
	ebx = gregs[REG_EBX];
	ecx = gregs[REG_ECX];
	edx = gregs[REG_EDX];

	esi = gregs[REG_ESI];
	edi = gregs[REG_EDI];
	ebp = gregs[REG_EBP];
	esp = gregs[REG_ESP];

	trace_printf("Registers:\n");
	trace_printf(" eax: %08lx   ebx: %08lx   ecx: %08lx   edx: %08lx\n",
	       eax, ebx, ecx, edx);
	trace_printf(" esi: %08lx   edi: %08lx   ebp: %08lx   esp: %08lx\n",
	       esi, edi, ebp, esp);
}

#else

#define IP_REG REG_RIP
#define BP_REG REG_RBP
#define SP_REG REG_RSP

#define IP_REG_NAME "RIP"

static void show_stack(unsigned long *sp)
{
	unsigned int i;

	trace_printf("Stack:\n");

	for (i = 0; i < STACK_DUMP_DEPTH; i++) {
		if (i && (i % STACK_ENTRIES_PER_LINE) == 0)
			trace_printf("\n");

		trace_printf("%016lx ", sp[i]);
	}

	trace_printf("\n");
}

static void show_registers(gregset_t gregs)
{
	unsigned long rsp;
	unsigned long rax, rbx, rcx;
	unsigned long rdx, rsi, rdi;
	unsigned long rbp, r8,  r9;
	unsigned long r10, r11, r12;
	unsigned long r13, r14, r15;

	rsp = gregs[REG_RSP];

	rax = gregs[REG_RAX];
	rbx = gregs[REG_RBX];
	rcx = gregs[REG_RCX];

	rdx = gregs[REG_RDX];
	rsi = gregs[REG_RSI];
	rdi = gregs[REG_RDI];

	rbp = gregs[REG_RBP];
	r8  = gregs[REG_R8];
	r9  = gregs[REG_R9];

	r10 = gregs[REG_R10];
	r11 = gregs[REG_R11];
	r12 = gregs[REG_R12];

	r13 = gregs[REG_R13];
	r14 = gregs[REG_R14];
	r15 = gregs[REG_R15];

	trace_printf("Registers:\n");
	trace_printf(" rsp: %016lx\n", rsp);
	trace_printf(" rax: %016lx   rbx: %016lx   rcx: %016lx\n", rax, rbx, rcx);
	trace_printf(" rdx: %016lx   rsi: %016lx   rdi: %016lx\n", rdx, rsi, rdi);
	trace_printf(" rbp: %016lx   r8:  %016lx   r9:  %016lx\n", rbp, r8,  r9);
	trace_printf(" r10: %016lx   r11: %016lx   r12: %016lx\n", r10, r11, r12);
	trace_printf(" r13: %016lx   r14: %016lx   r15: %016lx\n", r13, r14, r15);
}
#endif

void print_backtrace_and_die(int sig, siginfo_t *info, void *secret)
{
	ucontext_t *uc = secret;
	unsigned long ip, bp, sp, addr;

	ip	= uc->uc_mcontext.gregs[IP_REG];
	bp	= uc->uc_mcontext.gregs[BP_REG];
	sp	= uc->uc_mcontext.gregs[SP_REG];
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

	show_registers(uc->uc_mcontext.gregs);

	show_stack((void *) sp);

	show_code((void *) ip);

	print_trace_from(ip, (void *) bp);

	trace_flush();

	abort();
}
