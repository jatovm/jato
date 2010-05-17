#ifndef __VM_MONITOR_H
#define __VM_MONITOR_H

#include "lib/list.h"

#include "arch/atomic.h"

#include <semaphore.h>
#include <pthread.h>

struct vm_exec_env;

/*
 * Structure used in relaxed-lock protocol for monitor locking.
 * Locking thread acquires the lock by placing pointer to its
 * vm_monitor_record in object.monitor_record. This process is called
 * inflation. During unlocking, when there are no waiting threads, the
 * thread reclaims this structure and sets object.monitor_record back to
 * NULL (defaltion). When there are threads blocked on monitor then the owner
 * abandons the structure, sets .owner field to NULL and wakes one thread.
 *
 * Each thread manages a pool of these records.
 *
 * For more details see David Dice's work: "Implementing Fast Java
 * Monitors with Relaxed-Locks".
 */
struct vm_monitor_record {
	/* Holds pointer to struct vm_exec_env */
	void			*owner;
	atomic_t		nr_blocked;
	atomic_t		nr_waiting;
	atomic_t		candidate;
	int			lock_count;
	struct list_head	ee_free_list_node;
	sem_t			sem;
	pthread_mutex_t		notify_mutex;
	pthread_cond_t		notify_cond;
};

int vm_object_lock(struct vm_object *self);
int vm_object_unlock(struct vm_object *self);
int vm_object_wait(struct vm_object *self);
int vm_object_timed_wait(struct vm_object *self, uint64_t ms, int ns);
int vm_object_notify(struct vm_object *self);
int vm_object_notify_all(struct vm_object *self);

#endif
