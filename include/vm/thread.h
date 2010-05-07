#ifndef __VM_THREAD_H
#define __VM_THREAD_H

#include "lib/list.h"

#include "vm/object.h"

#include <stdio.h> /* for NOT_IMPLEMENTED */
#include <pthread.h>

struct vm_object;

enum vm_thread_state {
	VM_THREAD_STATE_BLOCKED,
	VM_THREAD_STATE_NEW,
	VM_THREAD_STATE_RUNNABLE,
	VM_THREAD_STATE_TERMINATED,
	VM_THREAD_STATE_TIMED_WAITING,
	VM_THREAD_STATE_WAITING,
};

enum thread_state {
	THREAD_STATE_CONSISTENT,
	THREAD_STATE_INCONSISTENT,
};

struct vm_thread {
	pthread_mutex_t mutex;

	/* instance of java.lang.VMThread associated with current thread */
	struct vm_object *vmthread;

	pthread_t posix_id;
	enum vm_thread_state state;
	struct list_head list_node;
	bool interrupted;
	struct vm_monitor *wait_mon;
	enum thread_state thread_state;
};

struct vm_exec_env {
	struct vm_thread *thread;
};

unsigned int vm_nr_threads(void);

extern __thread struct vm_exec_env current_exec_env;

static inline struct vm_exec_env *vm_get_exec_env(void)
{
	return &current_exec_env;
}

static inline struct vm_thread *vm_thread_self(void)
{
	return vm_get_exec_env()->thread;
}

int init_threading(void);
int vm_thread_start(struct vm_object *vmthread);
void vm_thread_wait_for_non_daemons(void);
void vm_thread_set_state(struct vm_thread *thread, enum vm_thread_state state);
struct vm_object *vm_thread_get_java_thread(struct vm_thread *thread);
char *vm_thread_get_name(struct vm_thread *thread);
bool vm_thread_is_interrupted(struct vm_thread *thread);
bool vm_thread_interrupted(struct vm_thread *thread);
void vm_thread_interrupt(struct vm_thread *thread);
void vm_thread_yield(void);
void vm_lock_thread_count(void);
void vm_unlock_thread_count(void);

extern struct list_head thread_list;
extern pthread_mutex_t threads_mutex;

#define vm_thread_for_each(this) list_for_each_entry(this, &thread_list, list_node)

#endif
