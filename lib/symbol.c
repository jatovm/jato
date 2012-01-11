/*
 * Copyright (c) 2012 Pekka Enberg
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
