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

/* get REG_EIP from ucontext.h */
#include <ucontext.h>

extern char *exe_name;

static asymbol *lookup_symbol(asymbol ** symbols, int nr_symbols,
			      const char *symbol_name)
{
	int i;

	for (i = 0; i < nr_symbols; i++) {
		asymbol *symbol = symbols[i];

		if (!strcmp(bfd_asymbol_name(symbol), symbol_name))
			return symbol;
	}
	return NULL;
}

bool show_exe_function(void *addr)
{
	bool ret;
	const char *function_name;
	bfd_vma symbol_offset;
	bfd_vma symbol_start;
	const char *filename;
	asection *sections;
	unsigned int line;
	asymbol **symbols;
	asymbol *symbol;
	int nr_symbols;
	bfd *abfd;
	int size;

	symbols = NULL;

	bfd_init();

	abfd = bfd_openr(exe_name, NULL);
	if (!abfd)
		goto failed;

	if (!bfd_check_format(abfd, bfd_object))
		goto failed;

	size = bfd_get_symtab_upper_bound(abfd);
	if (!size)
		goto failed;

	symbols = malloc(size);
	if (!symbols)
		goto failed;

	nr_symbols = bfd_canonicalize_symtab(abfd, symbols);

	sections = bfd_get_section_by_name(abfd, ".debug_info");
	if (!sections)
		goto failed;

	if (!bfd_find_nearest_line
	    (abfd, sections, symbols, (unsigned long) addr, &filename,
	     &function_name, &line))
		goto failed;

	if (!function_name)
		goto failed;

	symbol = lookup_symbol(symbols, nr_symbols, function_name);
	if (!symbol)
		goto failed;

	symbol_start = bfd_asymbol_value(symbol);
	symbol_offset = (unsigned long) addr - symbol_start;

	trace_printf("%s+%llx (%s:%i)\n",
		function_name, (long long) symbol_offset, filename, line);
	ret = true;

out:
	free(symbols);

	if (abfd)
		bfd_close(abfd);
	return ret;

failed:
	ret = false;
	goto out;
}

void show_function(void *addr)
{
	if (show_exe_function(addr))
		return;

	trace_printf("<unknown>\n");
}

static unsigned long get_greg(gregset_t gregs, int reg)
{
	return (unsigned long)gregs[reg];
}

#ifndef CONFIG_X86_64

#define IP_REG REG_EIP
#define BP_REG REG_EBP

#define IP_REG_NAME "EIP"

static void show_registers(gregset_t gregs)
{
	unsigned long eax, ebx, ecx, edx;
	unsigned long esi, edi, ebp, esp;

	eax = get_greg(gregs, REG_EAX);
	ebx = get_greg(gregs, REG_EBX);
	ecx = get_greg(gregs, REG_ECX);
	edx = get_greg(gregs, REG_EDX);

	esi = get_greg(gregs, REG_ESI);
	edi = get_greg(gregs, REG_EDI);
	ebp = get_greg(gregs, REG_EBP);
	esp = get_greg(gregs, REG_ESP);

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

	rsp = get_greg(gregs, REG_RSP);

	rax = get_greg(gregs, REG_RAX);
	rbx = get_greg(gregs, REG_RBX);
	rcx = get_greg(gregs, REG_RCX);

	rdx = get_greg(gregs, REG_RDX);
	rsi = get_greg(gregs, REG_RSI);
	rdi = get_greg(gregs, REG_RDI);

	rbp = get_greg(gregs, REG_RBP);
	r8  = get_greg(gregs, REG_R8);
	r9  = get_greg(gregs, REG_R9);

	r10 = get_greg(gregs, REG_R10);
	r11 = get_greg(gregs, REG_R11);
	r12 = get_greg(gregs, REG_R12);

	r13 = get_greg(gregs, REG_R13);
	r14 = get_greg(gregs, REG_R14);
	r15 = get_greg(gregs, REG_R15);

	trace_printf("Registers:\n");
	trace_printf(" rsp: %016lx\n", rsp);
	trace_printf(" rax: %016lx   ebx: %016lx   ecx: %016lx\n", rax, rbx, rcx);
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
	default:
		trace_printf("Signal %d at %s %08lx\n", sig, IP_REG_NAME, eip);
		break;
	};
	show_registers(uc->uc_mcontext.gregs);

	print_trace_from(eip, (void *) ebp);

	trace_flush();

	exit(1);
}
