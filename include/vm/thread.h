#ifndef __VM_THREAD_H
#define __VM_THREAD_H

#include "lib/list.h"

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

struct vm_thread {
	pthread_mutex_t mutex;

	/* instance of java.lang.VMThread associated with current thread */
	struct vm_object *vmthread;

	pthread_t posix_id;
	enum vm_thread_state state;
	struct list_head list_node;
};

struct vm_exec_env {
	struct vm_thread *thread;
};

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

#endif
