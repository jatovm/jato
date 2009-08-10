#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#include "lib/guard-page.h"
#include "vm/die.h"
#include "vm/gc.h"

void *gc_safepoint_page;

static pthread_mutex_t safepoint_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Protected by safepoint_mutex */
static unsigned int nr_threads = 0;

void gc_init(void)
{
	gc_safepoint_page = alloc_guard_page(false);
	if (!gc_safepoint_page)
		die("Couldn't allocate GC safepoint guard page");
}

void gc_attach_thread(void)
{
	pthread_mutex_lock(&safepoint_mutex);
	++nr_threads;
	pthread_mutex_unlock(&safepoint_mutex);
}

void gc_detach_thread(void)
{
	assert(nr_threads > 0);

	pthread_mutex_lock(&safepoint_mutex);
	--nr_threads;
	pthread_mutex_unlock(&safepoint_mutex);
}

void gc_safepoint(void)
{
}
