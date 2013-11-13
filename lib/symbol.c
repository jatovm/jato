/*
 * Copyright (c) 2012 Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/symbol.h"

#include <execinfo.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <bfd.h>

static asection *sections;
static asymbol **symbols;
static int nr_symbols;
static bfd *abfd;

bool symbol_init(void)
{
	int size;

	if (abfd)
		return true;

	bfd_init();

	abfd = bfd_openr("/proc/self/exe", NULL);
	if (!abfd)
		return false;

	if (!bfd_check_format(abfd, bfd_object))
		return false;

	size = bfd_get_symtab_upper_bound(abfd);
	if (!size)
		return false;

	symbols = malloc(size);
	if (!symbols)
		return false;

	nr_symbols = bfd_canonicalize_symtab(abfd, symbols);

	sections = bfd_get_section_by_name(abfd, ".debug_info");
	if (!sections)
		return false;

	return true;
}

void symbol_exit(void)
{
	free(symbols);

	if (abfd)
		bfd_close(abfd);
}

static char *symbol_lookup_glibc(void *addr, char *buf, unsigned long len)
{
	void *addrs[] = { addr };
	char **symbols;

	symbols = backtrace_symbols(addrs, 1);
	if (!symbols)
		return NULL;

	snprintf(buf, len, "%s", symbols[0]);

	free(symbols);
	return buf;
}

static asymbol *
symbol_lookup_bfd(asymbol ** symbols, int nr_symbols, const char *symbol_name)
{
	int i;

	for (i = 0; i < nr_symbols; i++) {
		asymbol *symbol = symbols[i];

		if (!strcmp(bfd_asymbol_name(symbol), symbol_name))
			return symbol;
	}
	return NULL;
}

char *symbol_lookup(unsigned long addr, char *buf, unsigned long len)
{
	const char *function_name;
	bfd_vma symbol_offset;
	bfd_vma symbol_start;
	const char *filename;
	unsigned int line;
	asymbol *symbol;

	if (!symbol_init())
		goto failed;

	if (!bfd_find_nearest_line(abfd, sections, symbols, (unsigned long) addr, &filename, &function_name, &line))
		goto failed;

	if (!function_name)
		goto failed;

	symbol = symbol_lookup_bfd(symbols, nr_symbols, function_name);
	if (!symbol)
		goto failed;

	symbol_start = bfd_asymbol_value(symbol);
	symbol_offset = (unsigned long) addr - symbol_start;

	snprintf(buf, len, "%s+%llx (%s:%i)", function_name, (long long) symbol_offset, filename, line);

	return buf;

failed:
	/*
	 * If above steps failed then try to obtain the symbol description with
	 * backtrace_symbols().
	 */
	return symbol_lookup_glibc((void *) addr, buf, len);
}
