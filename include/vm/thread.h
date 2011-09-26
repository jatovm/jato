#ifndef __VM_THREAD_H
#define __VM_THREAD_H

#include "lib/list.h"

#include "arch/atomic.h"

#include <stdio.h> /* for NOT_IMPLEMENTED */
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>

struct vm_object;

enum vm_thread_state {
	VM_THREAD_STATE_BLOCKED,
	VM_THREAD_STATE_NEW,
	VM_THREAD_STATE_RUNNABLE,
	VM_THREAD_STATE_TERMINATED,
	VM_THREAD_STATE_TIMED_WAITING,
	VM_THREAD_STATE_WAITING,
	VM_THREAD_STATE_CONSISTENT,
	VM_THREAD_STATE_INCONSISTENT,
};

struct vm_thread {
	pthread_mutex_t mutex;

	/* Instance of java.lang.VMThread associated with current
	 * thread. This reference must not prevent garbage collection. */
	struct vm_object *vmthread;

	pthread_t posix_id;
	atomic_t state;
	struct list_head list_node;
	bool interrupted;

	/* Points to object on which thread is waiting or NULL */
	struct vm_object *waiting_mon;

	/* Should be accessed only with vm_thread_(set|get)_state() */
	enum vm_thread_state thread_state;

	/* Needed by sun.misc.Unsafe.park() */
	pthread_mutex_t park_mutex;
	pthread_cond_t park_cond;
	bool unpark_called;

	struct vm_exec_env *ee;
};

struct vm_exec_env {
	struct vm_thread *thread;
	struct list_head free_monitor_recs;

	/*
	 * Holds a reference to exception that has been signalled.  This
	 * pointer is cleared when handler is executed or
	 * clear_exception() is called.
	 */
	struct vm_object *exception;

	/* Used by classloader when tracing with -Xtrace:classloader */
	int trace_classloader_level;

	/* A semaphore flag used by GC */
	sig_atomic_t in_safepoint;
};

unsigned int vm_nr_threads(void);

extern pthread_key_t current_exec_env_key;

static inline struct vm_exec_env *vm_get_exec_env(void)
{
	return pthread_getspecific(current_exec_env_key);
}

static inline struct vm_thread *vm_thread_self(void)
{
	struct vm_exec_env *current_exec_env = vm_get_exec_env();

	if (current_exec_env == NULL)
		return NULL;

	return current_exec_env->thread;
}

void init_exec_env(void);
int init_threading(void);
int vm_thread_start(struct vm_object *vmthread);
void vm_thread_wait_for_non_daemons(void);
void vm_thread_set_state(struct vm_thread *thread, enum vm_thread_state state);
enum vm_thread_state vm_thread_get_state(struct vm_thread *thread);
struct vm_object *vm_thread_get_java_thread(struct vm_thread *thread);
char *vm_thread_get_name(struct vm_thread *thread);
bool vm_thread_is_interrupted(struct vm_thread *thread);
bool vm_thread_interrupted(struct vm_thread *thread);
void vm_thread_interrupt(struct vm_thread *thread);
void vm_thread_yield(void);
struct vm_thread *vm_thread_from_vmthread(struct vm_object *vmthread);
struct vm_thread *vm_thread_from_java_thread(struct vm_object *jthread);
void vm_lock_thread_count(void);
void vm_unlock_thread_count(void);
void vm_thread_collect_vmthread(struct vm_object *object);

extern struct list_head thread_list;
extern pthread_mutex_t threads_mutex;

#define vm_thread_for_each(this) list_for_each_entry(this, &thread_list, list_node)

#endif
