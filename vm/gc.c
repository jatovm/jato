#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>

#include "arch/registers.h"
#include "lib/guard-page.h"
#include "vm/thread.h"
#include "vm/stdlib.h"
#include "vm/die.h"
#include "vm/gc.h"

void *gc_safepoint_page;

static pthread_mutex_t safepoint_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Protected by safepoint_mutex */
static pthread_cond_t everyone_in_cond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t everyone_out_cond = PTHREAD_COND_INITIALIZER;
static unsigned int nr_exiting_safepoint;
static unsigned int nr_in_safepoint;

static pthread_cond_t can_continue_cond = PTHREAD_COND_INITIALIZER;
static bool can_continue = true;

bool verbose_gc;
bool gc_enabled;

void gc_init(void)
{
	gc_safepoint_page = alloc_guard_page(false);
	if (!gc_safepoint_page)
		die("Couldn't allocate GC safepoint guard page");
}

void *gc_alloc(size_t size)
{
	struct register_state regs;

	save_registers(&regs);

	if (gc_enabled)
		gc_start(&regs);

	return zalloc(size);
}

void gc_attach_thread(void)
{
}

void gc_detach_thread(void)
{
}

static void hide_safepoint_guard_page(void)
{
	hide_guard_page(gc_safepoint_page);
}

static void unhide_safepoint_guard_page(void)
{
	unhide_guard_page(gc_safepoint_page);
}

static void do_gc_reclaim(void)
{
	if (verbose_gc)
		fprintf(stderr, "[GC]\n");

	/* TODO: Do main GC work here. */
}

/* Callers must hold safepoint_mutex */
static void gc_safepoint_exit(void)
{
	assert(nr_exiting_safepoint > 0);

	/*
	 * Don't let threads stop the world until everyone is out of their
	 * safepoint.
	 */
	if (--nr_exiting_safepoint == 0)
		pthread_cond_broadcast(&everyone_out_cond);
}

/* Callers must hold safepoint_mutex */
static void do_gc_safepoint(void)
{
	/* Only the GC thread will be waiting for this. */
	if (++nr_in_safepoint == vm_nr_threads_running())
		pthread_cond_signal(&everyone_in_cond);

	/* Block until GC has finished */
	while (!can_continue)
		pthread_cond_wait(&can_continue_cond, &safepoint_mutex);

	assert(nr_in_safepoint > 0);
	--nr_in_safepoint;
}

void gc_safepoint(struct register_state *regs)
{
	pthread_mutex_lock(&safepoint_mutex);

	do_gc_safepoint();

	gc_safepoint_exit();

	pthread_mutex_unlock(&safepoint_mutex);
}

/*
 * This is the main entrypoint to the stop-the-world GC.
 */
void gc_start(struct register_state *regs)
{
	pthread_mutex_lock(&safepoint_mutex);

	/* Don't deadlock during early boostrap. */
	if (vm_nr_threads_running() == 0)
		goto out_unlock;

	/*
	 * Wait until all threads have exited their safepoint before stopping
	 * the world again.
	 */
	while (nr_exiting_safepoint > 0)
		pthread_cond_wait(&everyone_out_cond, &safepoint_mutex);

	/*
	 * If someone stopped the world before us, put the current thread in a
	 * safepoint.
	 */
	if (nr_in_safepoint != 0) {
		do_gc_safepoint();
		goto out_exit_safepoint;
	}

	/* Only one thread can stop the world at a time. */
	assert(can_continue);

	++nr_in_safepoint;
	can_continue = false;
	hide_safepoint_guard_page();

	/* Wait for all other threads to enter a safepoint. */
	while (nr_in_safepoint != vm_nr_threads_running())
		pthread_cond_wait(&everyone_in_cond, &safepoint_mutex);

	/* At this point, we know that everyone is in the safepoint. */
	unhide_safepoint_guard_page();

	do_gc_reclaim();

	/* Resume other threads */
	assert(nr_in_safepoint > 0);
	nr_exiting_safepoint = nr_in_safepoint--;
	can_continue = true;
	pthread_cond_broadcast(&can_continue_cond);

out_exit_safepoint:
	gc_safepoint_exit();
out_unlock:
	pthread_mutex_unlock(&safepoint_mutex);
}
