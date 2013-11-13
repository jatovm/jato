/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/guard-page.h"
#include "vm/alloc.h"
#include "vm/die.h"

#include <sys/mman.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>

/**
 * alloc_offset_guard - allocates a memory region with two
 *   sub-regions. The first one contains addresses from [0:valid_size[
 *   with access allowed. The second region contains addresses
 *   [valid_size:valid_size + overflow_size[ and access to this region
 *   triggers SIGSEGV.
 */
void *alloc_offset_guard(unsigned long valid_size, unsigned long overflow_size)
{
	int valid_pages, overflow_pages;
	int page_size;
	void *result;

	assert(overflow_size > 0);

	page_size = getpagesize();

	valid_pages = (valid_size + page_size - 1) / page_size;
	overflow_pages = (overflow_size + page_size - 1) / page_size;

	result = alloc_pages(valid_pages + overflow_pages);

	mprotect(result + valid_pages * page_size,
		 overflow_pages * page_size,
		 PROT_NONE);

	unsigned long offset = page_size - valid_size % page_size;
	if (offset == (unsigned long) page_size)
		offset = 0;

	return result + offset;
}

void *alloc_guard_page(bool hidden)
{
	void *p = alloc_pages(1);
	if (!p)
		return NULL;

	if (hidden) {
		if (hide_guard_page(p))
			return NULL;
	}

	return p;
}

int hide_guard_page(void *page_ptr)
{
	return mprotect(page_ptr, getpagesize(), PROT_NONE);
}

int unhide_guard_page(void *page_ptr)
{
	return mprotect(page_ptr, getpagesize(), PROT_READ|PROT_WRITE);
}
