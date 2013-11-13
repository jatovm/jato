/*
 * Copyright (c) 2006-2008  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/backtrace.h"

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

/* get REG_EIP from ucontext.h */
#include <ucontext.h>

static asymbol *lookup_symbol(asymbol ** symbols, int nr_symbols, const char *symbol_name)
{
	return NULL;
}

static bool show_backtrace_function(void *addr, struct string *str)
{
	void * buf[] = { addr };
	char **symbols;

	symbols = backtrace_symbols(buf, 1);
	if (symbols == NULL) {
		return false;
	}

	str_append(str, "%s\n", symbols[0]);

	free(symbols);
	return true;
}

bool show_exe_function(void *addr, struct string *str)
{
	return false;
}

void show_function(void *addr)
{
	struct string *str;

	str = alloc_str();

	if (show_exe_function(addr, str)) {
		trace_printf("%s", str->value);
	} else {
		trace_printf("<unknown>\n");
	}

	free_str(str);
}

#ifndef CONFIG_X86_64

#define IP_REG REG_EIP
#define BP_REG REG_EBP

#define IP_REG_NAME "EIP"

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

#define IP_REG_NAME "RIP"

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
	unsigned long eip, ebp, addr;

	eip	= uc->uc_mcontext.gregs[IP_REG];
	ebp     = uc->uc_mcontext.gregs[BP_REG];
	addr	= (unsigned long) info->si_addr;

	switch (sig) {
	case SIGSEGV:
		trace_printf("SIGSEGV at %s %08lx while accessing memory address %08lx.\n",
			IP_REG_NAME, eip, addr);
		break;
	case SIGILL:
		trace_printf("SIGILL at %s %08lx\n", sig, IP_REG_NAME, eip);
		break;
	default:
		trace_printf("Signal %d at %s %08lx\n", sig, IP_REG_NAME, eip);
		break;
	};
	show_registers(uc->uc_mcontext.gregs);

	print_trace_from(eip, (void *) ebp);

	trace_flush();

	abort();
}
