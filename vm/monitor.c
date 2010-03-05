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

#include "jit/exception.h"

#include "vm/object.h"
#include "vm/preload.h"

static pthread_mutexattr_t monitor_mutexattr;

int init_vm_monitors(void)
{
	int err;

	err = pthread_mutexattr_init(&monitor_mutexattr);
	if (err)
		return -err;

	err = pthread_mutexattr_settype(&monitor_mutexattr,
		PTHREAD_MUTEX_RECURSIVE);
	if (err)
		return -err;

	return 0;
}

int vm_monitor_init(struct vm_monitor *mon)
{
	if (pthread_mutex_init(&mon->owner_mutex, NULL))
		return -1;

	if (pthread_mutex_init(&mon->mutex, &monitor_mutexattr))
		return -1;

	if (pthread_cond_init(&mon->cond, NULL))
		return -1;

	mon->owner = NULL;
	mon->lock_count = 0;

	return 0;
}

struct vm_thread *vm_monitor_get_owner(struct vm_monitor *mon)
{
	struct vm_thread *owner;

	pthread_mutex_lock(&mon->owner_mutex);
	owner = mon->owner;
	pthread_mutex_unlock(&mon->owner_mutex);

	return owner;
}

void vm_monitor_set_owner(struct vm_monitor *mon, struct vm_thread *owner)
{
	pthread_mutex_lock(&mon->owner_mutex);
	mon->owner = owner;
	pthread_mutex_unlock(&mon->owner_mutex);
}

int vm_monitor_lock(struct vm_monitor *mon)
{
	struct vm_thread *self;
	int err;

	self = vm_thread_self();
	err = 0;

	if (pthread_mutex_trylock(&mon->mutex)) {
		/*
		 * XXX: according to Thread.getState() documentation thread
		 * state does not have to be precise, it's used rather
		 * for monitoring.
		 */
		vm_thread_set_state(self, VM_THREAD_STATE_BLOCKED);
		err = pthread_mutex_lock(&mon->mutex);
		vm_thread_set_state(self, VM_THREAD_STATE_RUNNABLE);
	}

	/* If err is non zero the lock has not been acquired. */
	if (!err) {
		vm_monitor_set_owner(mon, self);
		mon->lock_count++;
	}

	return err;
}

int vm_monitor_unlock(struct vm_monitor *mon)
{
	if (vm_monitor_get_owner(mon) != vm_thread_self()) {
		signal_new_exception(vm_java_lang_IllegalMonitorStateException,
				     NULL);
		return -1;
	}

	if (--mon->lock_count == 0)
		vm_monitor_set_owner(mon, NULL);

	int err = pthread_mutex_unlock(&mon->mutex);

	/* If err is non zero the lock has not been released. */
	if (err) {
		++mon->lock_count;
		vm_monitor_set_owner(mon, vm_thread_self());
	}

	return err;
}

static int vm_monitor_do_wait(struct vm_monitor *mon, struct timespec *timespec)
{
	struct vm_thread *self;
	int old_lock_count;
	bool interrupted;
	int err;

	if (vm_monitor_get_owner(mon) != vm_thread_self()) {
		signal_new_exception(vm_java_lang_IllegalMonitorStateException,
				     NULL);
		return -1;
	}

	old_lock_count = mon->lock_count;
	mon->lock_count = 0;
	vm_monitor_set_owner(mon, NULL);

	self = vm_thread_self();

	pthread_mutex_lock(&self->mutex);
	if (timespec != NULL)
		self->state = VM_THREAD_STATE_TIMED_WAITING;
	else
		self->state = VM_THREAD_STATE_WAITING;

	self->wait_mon = mon;
	interrupted = self->interrupted;
	pthread_mutex_unlock(&self->mutex);

	if (interrupted)
		err = 0;
	else if (timespec) {
		err = pthread_cond_timedwait(&mon->cond, &mon->mutex, timespec);
		if (err == ETIMEDOUT)
			err = 0;
	} else
		err = pthread_cond_wait(&mon->cond, &mon->mutex);

	pthread_mutex_lock(&self->mutex);
	self->state = VM_THREAD_STATE_RUNNABLE;
	self->wait_mon = NULL;
	pthread_mutex_unlock(&self->mutex);

	vm_monitor_set_owner(mon, self);
	mon->lock_count = old_lock_count;

	if (vm_thread_interrupted(self)) {
		signal_new_exception(vm_java_lang_InterruptedException, NULL);
		return -1;
	}

	return err;
}

int vm_monitor_timed_wait(struct vm_monitor *mon, long long ms, int ns)
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

	return vm_monitor_do_wait(mon, &timespec);
}

int vm_monitor_wait(struct vm_monitor *mon)
{
	return vm_monitor_do_wait(mon, NULL);
}

int vm_monitor_notify(struct vm_monitor *mon)
{
	if (vm_monitor_get_owner(mon) != vm_thread_self()) {
		signal_new_exception(vm_java_lang_IllegalMonitorStateException,
				     NULL);
		return -1;
	}

	return pthread_cond_signal(&mon->cond);
}

int vm_monitor_notify_all(struct vm_monitor *mon)
{
	if (vm_monitor_get_owner(mon) != vm_get_exec_env()->thread) {
		signal_new_exception(vm_java_lang_IllegalMonitorStateException,
				     NULL);
		return -1;
	}

	return pthread_cond_broadcast(&mon->cond);
}
