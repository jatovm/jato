/*
 * Copyright (c) 2006,2009  Pekka Enberg
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

#include <sys/mman.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "arch/text.h"
#include "jit/text.h"

#include "vm/system.h"
#include "vm/alloc.h"
#include "vm/die.h"

#define MAX_TEXT_SIZE (256 * 1024 * 1024) /* 256 MB */

static pthread_mutex_t jit_text_mutex = PTHREAD_MUTEX_INITIALIZER;
static unsigned long jit_text_offset;
static void *jit_text;

void jit_text_init(void)
{
	jit_text = mmap(NULL, MAX_TEXT_SIZE,
			PROT_READ | PROT_WRITE | PROT_EXEC,
			MAP_PRIVATE | MAP_ANONYMOUS | TEXT_MAP_FLAGS, -1, 0);
	if (jit_text == MAP_FAILED)
		die("mmap");
}

bool is_jit_text(void *p)
{
	return p >= jit_text && p <= (jit_text + MAX_TEXT_SIZE);
}

void jit_text_lock(void)
{
	if (pthread_mutex_lock(&jit_text_mutex) != 0)
		die("pthread_mutex_lock");
}

void jit_text_unlock(void)
{
	if (pthread_mutex_unlock(&jit_text_mutex) != 0)
		die("pthread_mutex_lock");
}

void *jit_text_ptr(void)
{
	return jit_text + jit_text_offset;
}

void jit_text_reserve(size_t size)
{
	jit_text_offset += ALIGN(size, TEXT_ALIGNMENT);

	assert(jit_text_offset < MAX_TEXT_SIZE);
}

void *alloc_pages(int n)
{
	int page_size;
	void *p;

	page_size = getpagesize();

	if (posix_memalign(&p, page_size, page_size * n))
		return NULL;

	return p;
}
