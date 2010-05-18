/*
 * Copyright (C) 2009 Tomasz Grabiec
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

#include <errno.h>
#include <pthread.h>

#include "arch/memory.h"
#include "arch/atomic.h"

#include "jit/exception.h"

#include "vm/object.h"
#include "vm/preload.h"
#include "vm/thread.h"
#include "vm/errors.h"

/*
 * Get new monitor record with .owner set to the current execution
 * environment and .lock_count set to 1.
 */
static struct vm_monitor_record *get_monitor_record(void)
{
	struct vm_monitor_record *record;
	struct vm_exec_env *ee;

	ee = vm_get_exec_env();

	if (!list_is_empty(&ee->free_monitor_recs)) {
		record = list_first_entry(&ee->free_monitor_recs,
				       struct vm_monitor_record,
				       ee_free_list_node);
		list_del(&record->ee_free_list_node);
		return record;
	}

	record = malloc(sizeof *record);
	if (!record)
		return NULL;

	record->owner		= ee;
	record->lock_count	= 1;

	atomic_set(&record->nr_blocked, 0);
	INIT_LIST_HEAD(&record->ee_free_list_node);
	sem_init(&record->sem, 0, 0);
	atomic_set(&record->candidate, 0);

	return record;
}

/*
 * Puts monitor record back to the pool
 */
static void put_monitor_record(struct vm_monitor_record *record)
{
	struct vm_exec_env *ee = vm_get_exec_env();
	list_add(&record->ee_free_list_node, &ee->free_monitor_recs);
}

/*
 * Wakes one thread waiting on monitor
 */
static inline void wake_one(struct vm_monitor_record *record)
{
	if (atomic_cmpxchg(&record->candidate, 1, 0) == 1) {
		sem_post(&record->sem);
	}
}

/*
 * Block current thread on monitor
 */
static inline void wait(struct vm_monitor_record *record)
{
	struct vm_thread *self = vm_thread_self();
	vm_thread_set_state(self, VM_THREAD_STATE_BLOCKED);
	sem_wait(&record->sem);
	vm_thread_set_state(self, VM_THREAD_STATE_RUNNABLE);
}

static inline
int owner_check(struct vm_object *object, struct vm_monitor_record **record_p)
{
	struct vm_monitor_record *record;

	/*
	 * Both atomic_read() calls do not need a memory barrier.
	 * If current thread owns the monitor then the first read will return
	 * non-null value and the second read will return current exec env because
	 * those values were set by this thread.
	 */

	record	= object->monitor_record;
	if (record && record->owner == vm_get_exec_env()) {
		*record_p = record;
		return 0;
	}

	signal_new_exception(vm_java_lang_IllegalMonitorStateException, NULL);
	return -1;
}

/*
 * Acquire the lock on object's monitor. This implementation uses the
 * relaxed-lock protocol based on David Dice's work: "Implementing
 * Fast Java Monitors with Relaxed-Locks".
 *
 */
int vm_object_lock(struct vm_object *self)
{
	struct vm_monitor_record *old_record;
	struct vm_exec_env *ee;

	ee = vm_get_exec_env();

	while (true) {
		old_record	= self->monitor_record;

		if (!old_record) {
			struct vm_monitor_record *record = get_monitor_record();
			if (!record) {
				throw_oom_error();
				return -1;
			}

			if (!cmpxchg_ptr(&self->monitor_record, NULL, record)) {
				return 0;
			}

			put_monitor_record(record);
			continue;
		}

		/* Check if recursive lock. No need for memory barrier
		 * here because if current thread unlocked the monitor
		 * and is no longer its owner then atomic_read() will
		 * not return current exec environment. */
		if (old_record->owner == ee) {
			old_record->lock_count++;
			return 0;
		}

		/* Slowpath... */

		atomic_inc(&old_record->nr_blocked);

		/* This barrier is paired with the barrier in
		 * unlocking thread after deflation. */
		smp_mb__after_atomic_inc();

		while (self->monitor_record == old_record) {

			/*
			 * The unlocking thread checks for it in
			 * wake_one() and if it is not set then it
			 * does not do sem_post(). At the time
			 * unlocking thread executes wake_one() the
			 * .owner field is already cleared so the
			 * cmpxchg_ptr() will succeed and this
			 * thread will not block.
			 *
			 * We don't need a memory barrier after the write
			 * because following cmpxchg_ptr() implies one.
			 */
			atomic_set(&old_record->candidate, 1);

			if (!cmpxchg_ptr(&old_record->owner, NULL, ee)) {
				atomic_dec(&old_record->nr_blocked);
				old_record->lock_count = 1;
				return 0;
			}

			wait(old_record);

			/* We want to see that deflation happen before
			 * unlocking thread wakes us up. Otherwise we
			 * will block again and unlocking thread will
			 * deadlock. */
			smp_rmb();
		}

		atomic_dec(&old_record->nr_blocked);

		/* Paired with smp_rmb() in unlocking thread's flushing loop */
		smp_mb__after_atomic_dec();
	}
}

/*
 * Release the lock on object's monitor.
 */
int vm_object_unlock(struct vm_object *self)
{
	struct vm_monitor_record *record;

	if (owner_check(self, &record))
		return -1;

	if (record->lock_count > 1) {
		record->lock_count--;
		smp_mb(); /* Required by java memory model */
		return 0;
	}

	if (atomic_read(&record->nr_blocked) > 0) {
		record->owner	= NULL;

		/* Order above write and read in a locking thread's
		 * while header so that it won't block after it's
		 * woken up. */
		smp_mb();

		wake_one(record);
		return 0;
	}

	/* Defalte monitor. */
	self->monitor_record	= NULL;

	/* We must ensure that when locking thread will read
	 * .monitor_record == NULL in the while header this thread
	 * sees the effect of incrementation of .nr_blocked. */
	smp_mb();

	int nr_to_wake = atomic_read(&record->nr_blocked);

	/*
	 * If it shows up that some thread entered a slowpath
	 * between the last check and deflation then we must
	 * notify those threads that the record has been "flushed"
	 * and they must try again.
	 *
	 * Note that after .monitor_record has been set to null
	 * no new threads will block on semaphore.
	 */
	if (nr_to_wake > 0) {
		/* Flush. It is possible that locking thread which
		 * incremented .nr_blocked will escape without
		 * blocking itself on the semaphore because it will
		 * detect deflation. In this case the semaphore will
		 * be incremented more times than needed to wake
		 * threads up. This is not harmful because locking
		 * threads call wait() in a loop trying to acquire the
		 * lock, and the semaphore will be quickly decremented. */
		for (int i = 0; i < nr_to_wake; i++)
			sem_post(&record->sem);

		/* We must wait for blocked threads to ACK the flush
		 * before this record can be put back to the pool and
		 * reused. */
		while (atomic_read(&record->nr_blocked) > 0) ;
	}

	put_monitor_record(record);
	return 0;
}

static int vm_object_do_wait(struct vm_object *self, struct timespec *timespec)
{
	struct vm_monitor_record *record;
	struct vm_thread *thread_self;
	bool interrupted;
	int old_lock_count;
	int err;

	if (owner_check(self, &record))
		return -1;

	thread_self = vm_thread_self();

	pthread_mutex_lock(&self->notify_mutex);

	pthread_mutex_lock(&thread_self->mutex);
	interrupted = thread_self->interrupted;
	thread_self->waiting_mon = self;
	pthread_mutex_unlock(&thread_self->mutex);

	enum vm_thread_state new_state =
		timespec ? VM_THREAD_STATE_TIMED_WAITING : VM_THREAD_STATE_WAITING;

	vm_thread_set_state(thread_self, new_state);

	/*
	 * We must unlock monitor after the lock on notify_mutex is acquired
	 * to avoid missed notify.
	 */
	old_lock_count = record->lock_count;
	record->lock_count = 1;
	vm_object_unlock(self);

	if (interrupted) {
		err = 0;
	} else if (timespec) {
		err = pthread_cond_timedwait(&self->notify_cond, &self->notify_mutex, timespec);
		if (err == ETIMEDOUT)
			err = 0;
	} else {
		err = pthread_cond_wait(&self->notify_cond, &self->notify_mutex);
	}

	pthread_mutex_unlock(&self->notify_mutex);

	pthread_mutex_lock(&thread_self->mutex);
	thread_self->waiting_mon = NULL;
	pthread_mutex_unlock(&thread_self->mutex);

	vm_thread_set_state(thread_self, VM_THREAD_STATE_RUNNABLE);

	vm_object_lock(self);

	record			= self->monitor_record;
	record->lock_count	= old_lock_count;

	if (vm_thread_interrupted(thread_self)) {
		signal_new_exception(vm_java_lang_InterruptedException, NULL);
		return -1;
	}

	return err;
}

int vm_object_timed_wait(struct vm_object *self, uint64_t ms, int ns)
{
	struct timespec timespec;

	/*
	 * XXX: we must use CLOCK_REALTIME here because
	 * pthread_cond_timedwait() uses this clock.
	 */
	clock_gettime(CLOCK_REALTIME, &timespec);

	timespec.tv_sec += ms / 1000;
	timespec.tv_nsec += (long)ns + (long)(ms % 1000) * 1000000l;

	if (timespec.tv_nsec >= 1000000000l) {
		timespec.tv_sec++;
		timespec.tv_nsec -= 1000000000l;
	}

	return vm_object_do_wait(self, &timespec);
}

int vm_object_wait(struct vm_object *self)
{
	return vm_object_do_wait(self, NULL);
}

int vm_object_notify(struct vm_object *self)
{
	struct vm_monitor_record *record;

	if (owner_check(self, &record))
		return -1;

	pthread_mutex_lock(&self->notify_mutex);
	pthread_cond_signal(&self->notify_cond);
	pthread_mutex_unlock(&self->notify_mutex);
	return 0;
}

int vm_object_notify_all(struct vm_object *self)
{
	struct vm_monitor_record *record;

	if (owner_check(self, &record)) {
		signal_new_exception(vm_java_lang_IllegalMonitorStateException, NULL);
		return -1;
	}

	pthread_mutex_lock(&self->notify_mutex);
	pthread_cond_broadcast(&self->notify_cond);
	pthread_mutex_unlock(&self->notify_mutex);
	return 0;
}
