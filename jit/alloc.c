/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for allocating special-purpose memory in the
 * JIT compiler. The executable memory allocation is shamelessly taken from
 * Parrot.
 */

#define _XOPEN_SOURCE 600

#include <system.h>
#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdlib.h>

/**
 *	alloc_exec - Allocate executable memory.
 *	@size: size of the allocated memory block.
 *
 *	This function allocates @size amouont of memory and ensures the region
 *	is executable. The allocated memory is released with free(3).
 *
 * 	Returns a pointer to executable memory region with at least @size
 * 	bytes. If allocation fails, returns NULL.
 */
void *alloc_exec(size_t size)
{
	int page_size;
	void *p;

	page_size = getpagesize();
	size = ALIGN(size, page_size);

	if (posix_memalign(&p, page_size, size))
		return NULL;
	mprotect(p, size, PROT_READ | PROT_WRITE | PROT_EXEC);

	return p;
}
