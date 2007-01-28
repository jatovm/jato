/*
 * Copyright (C) 2006-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <bfd.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* get REG_EIP from ucontext.h */
#define __USE_GNU
#include <ucontext.h>

char *exe_name;

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

static void show_function(void *addr)
{
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
	    (abfd, sections, symbols, (unsigned long)addr, &filename,
	     &function_name, &line))
		goto failed;

	symbol = lookup_symbol(symbols, nr_symbols, function_name);
	if (!symbol)
		goto failed;

	symbol_start = bfd_asymbol_value(symbol);
	symbol_offset = (unsigned long)addr - symbol_start;

	printf(" [<%08lx>] %s+%llx (%s:%i)\n", (unsigned long) addr, function_name,
	       symbol_offset, filename, line);

  out:
	free(symbols);

	if (abfd)
		bfd_close(abfd);
	return;

  failed:
	printf(" [<%08lx>] <unknown>\n", (unsigned long) addr);
	goto out;
}

/* Must be inline so this does not change the backtrace for
   bt_sighandler. */
static inline void __show_stack_trace(unsigned long start, void *caller)
{
	void *array[10];
	size_t size;
	size_t i;

	size = backtrace(array, 10);
	array[1] = caller;

	printf("Native stack trace:\n");
	for (i = start; i < size; i++)
		show_function(array[i]);
}

void print_trace(void)
{
	__show_stack_trace(0, NULL);
}

#ifdef CONFIG_X86_64
#define IP_REG REG_RIP
#define IP_REG_NAME "RIP"
#else
#define IP_REG REG_EIP
#define IP_REG_NAME "EIP"
#endif

static unsigned long get_greg(gregset_t gregs, int reg)
{
	return (unsigned long)gregs[reg];
}

#ifndef CONFIG_X86_64
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

	printf("Registers:\n");
	printf(" eax: %08lx   ebx: %08lx   ecx: %08lx   edx: %08lx\n",
	       eax, ebx, ecx, edx);
	printf(" esi: %08lx   edi: %08lx   ebp: %08lx   esp: %08lx\n",
	       esi, edi, ebp, esp);
}
#else
static void show_registers(gregset_t gregs)
{
}
#endif

void bt_sighandler(int sig, siginfo_t * info, void *secret)
{
	void *eip;
	ucontext_t *uc = secret;

	eip = (void *)uc->uc_mcontext.gregs[IP_REG];

	if (sig == SIGSEGV)
		printf
		    ("SIGSEGV at %s %08lx while accessing memory address %08lx.\n",
		     IP_REG_NAME, (unsigned long)eip,
		     (unsigned long)info->si_addr);
	else
		printf("Signal %d at %s %08lx\n", sig, IP_REG_NAME, (unsigned long)eip);

	show_registers(uc->uc_mcontext.gregs);
	__show_stack_trace(1, eip);
	exit(1);
}
