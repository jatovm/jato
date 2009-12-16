#include <assert.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>

#include "arch/memory.h"
#include "arch/registers.h"
#include "arch/signal.h"
#include "lib/guard-page.h"
#include "vm/thread.h"
#include "vm/stdlib.h"
#include "vm/die.h"
#include "vm/gc.h"
#include "vm/trace.h"

void *gc_safepoint_page;

static pthread_mutex_t gc_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gc_cond = PTHREAD_COND_INITIALIZER;

/* Protected by gc_mutex */
static unsigned int nr_threads;
static bool gc_started;

static sem_t safepoint_sem;

static sig_atomic_t can_continue;
static __thread sig_atomic_t in_safepoint;

static pthread_t gc_thread_id;

bool verbose_gc;
bool gc_enabled;

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

void gc_safepoint(struct register_state *regs)
{
	sigset_t mask;
	int sig;

	in_safepoint = true;
	sem_post(&safepoint_sem);

	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);

	rmb();
	while (!can_continue) {
		sigwait(&mask, &sig);
		rmb();
	}

	in_safepoint = false;
	sem_post(&safepoint_sem);
}

void suspend_handler(int sig, siginfo_t *si, void *ctx)
{
	/*
	 * Ignore this signal if we are already in safepoint. This
	 * happens when a thread executes a safepoint poll in JIT
	 * before the GC thread sends suspend signals.
	 */
	if (in_safepoint)
		return;

	/*
	 * Force native code to enter a safepoint and restart non-native
	 * threads. The latter will suspend themselves once they reach a GC
	 * point.
	 */

	if (signal_from_native(ctx)) {
		struct register_state thread_register_state;
		ucontext_t *uc = ctx;

		save_signal_registers(&thread_register_state, uc->uc_mcontext.gregs);
		gc_safepoint(&thread_register_state);
	}
}

static void gc_suspend_rest(void)
{
	struct vm_thread *thread;

	sem_init(&safepoint_sem, true, 1 - nr_threads);
	can_continue = false;
	wmb();

	vm_thread_for_each(thread) {
		if (pthread_kill(thread->posix_id, SIGUSR2) != 0)
			die("pthread_kill");
	}

	/* Wait for all threads to enter a safepoint. */
	sem_wait(&safepoint_sem);
}

static void gc_resume_rest(void)
{
	struct vm_thread *thread;

	sem_init(&safepoint_sem, true, 1 - nr_threads);

	can_continue = true;
	wmb();

	vm_thread_for_each(thread) {
		if (pthread_kill(thread->posix_id, SIGUSR2) != 0)
			die("pthread_kill");
	}

	/* Wait for all threads to leave a safepoint. */
	sem_wait(&safepoint_sem);
}

static void do_gc(void)
{
	vm_lock_thread_count();

	nr_threads = vm_nr_threads();

	/* Don't deadlock during early boostrap. */
	if (nr_threads == 0)
		goto out;

	hide_safepoint_guard_page();
	gc_suspend_rest();
	unhide_safepoint_guard_page();

	do_gc_reclaim();
	gc_resume_rest();

out:
	vm_unlock_thread_count();
}

static void *gc_thread(void *arg)
{
	pthread_mutex_lock(&gc_mutex);

	for (;;) {
		while (!gc_started)
			pthread_cond_wait(&gc_cond, &gc_mutex);

		do_gc();

		gc_started = false;
		pthread_cond_broadcast(&gc_cond);
	}

	pthread_mutex_unlock(&gc_mutex);
	return NULL;
}

/*
 * This wakes up the GC thread and suspends until garbage collection is done.
 */
static void gc_start(struct register_state *regs)
{
	pthread_mutex_lock(&gc_mutex);

	gc_started = true;
	pthread_cond_broadcast(&gc_cond);

	while (gc_started)
		pthread_cond_wait(&gc_cond, &gc_mutex);

	pthread_mutex_unlock(&gc_mutex);
}

void gc_init(void)
{
	gc_safepoint_page = alloc_guard_page(false);
	if (!gc_safepoint_page)
		die("Couldn't allocate GC safepoint guard page");

	if (pthread_create(&gc_thread_id, NULL, &gc_thread, NULL))
		die("Couldn't create GC thread");
}

void *gc_alloc(size_t size)
{
	struct register_state regs;

	save_registers(&regs);

	if (gc_enabled)
		gc_start(&regs);

	return zalloc(size);
}
