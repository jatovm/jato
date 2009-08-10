#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "lib/guard-page.h"
#include "vm/die.h"
#include "vm/gc.h"

void *gc_safepoint_page;

static pthread_mutex_t safepoint_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Protected by safepoint_mutex */
static pthread_cond_t everyone_in_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t everyone_out_cond = PTHREAD_COND_INITIALIZER;
static unsigned int nr_threads = 0;
static unsigned int nr_in_safepoint = 0;

static pthread_cond_t can_continue_cond = PTHREAD_COND_INITIALIZER;
static bool can_continue;

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

static void hide_safepoint_guard_page(void)
{
	hide_guard_page(gc_safepoint_page);
}

static void unhide_safepoint_guard_page(void)
{
	unhide_guard_page(gc_safepoint_page);
}

void gc_start(void)
{
	pthread_mutex_lock(&safepoint_mutex);

	assert(nr_in_safepoint == 0);

	can_continue = false;
	hide_safepoint_guard_page();

	while (nr_in_safepoint != nr_threads)
		pthread_cond_wait(&everyone_in_cond, &safepoint_mutex);

	/* At this point, we know that everyone is in the safepoint. */
	unhide_safepoint_guard_page();

	/* TODO: Do main GC work here. */

	/* Resume other threads */
	can_continue = true;
	pthread_cond_broadcast(&can_continue_cond);

	while (nr_in_safepoint != 0)
		pthread_cond_wait(&everyone_out_cond, &safepoint_mutex);

	pthread_mutex_unlock(&safepoint_mutex);
}

void gc_safepoint(void)
{
	pthread_mutex_lock(&safepoint_mutex);

	/* Only the GC thread will be waiting for this. */
	if (++nr_in_safepoint == nr_threads)
		pthread_cond_signal(&everyone_in_cond);

	/* Block until GC has finished */
	while (!can_continue)
		pthread_cond_wait(&can_continue_cond, &safepoint_mutex);

	/* Only the GC thread will be waiting for this. */
	if (--nr_in_safepoint == 0)
		pthread_cond_signal(&everyone_out_cond);

	pthread_mutex_unlock(&safepoint_mutex);
}
