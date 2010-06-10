#ifndef _LIB_COMPILE_LOCK_H
#define _LIB_COMPILE_LOCK_H

#include "arch/memory.h"
#include "arch/atomic.h"

#include "vm/thread.h"

#include <semaphore.h>

/*
 * Compilation lock status. The state graph for compilation lock
 * is as follows:
 *
 * (INITIAL) ---> (COMPILING) -----> (COMPILED_OK)
 *     ^             |        |
 *     |_____________|        \----> (COMPILED_ERRONOUS)
 *
 */
enum compile_lock_status {
	STATUS_INITIAL = 0,
	STATUS_COMPILING,

	/* below are final states. */
	STATUS_COMPILED_OK,
	STATUS_COMPILED_ERRONOUS,

	/* Pseudo state returned by compile_lock_enter() */
	STATUS_REENTER
};

struct compile_lock {
	/* compilation status */
	atomic_t status;

	/* Number of threads waiting for status update. */
	atomic_t nr_waiting;

	/* semaphore for threads waiting for status update. */
	sem_t wait_sem;

	/* True if lock is reentrant */
	bool reentrant;

	/* Compiling thread. Relevant only when reentrant. */
	struct vm_exec_env *compiling_ee;
};

int compile_lock_init(struct compile_lock *cl, bool reentrant);

/*
 * Enter compilation lock. Only one thread will be allowed.
 * If this method returns STATUS_COMPILING then this thread
 * is eligible for compilation and this thread only must
 * call compile_lock_leave() when compilation is done.
 *
 * If lock is reentrant and compilation is reentered then
 * returns STATUS_REENTER.
 */
enum compile_lock_status compile_lock_enter(struct compile_lock *cl);

/*
 * Finish compilation with given status. Only compiling thread
 * can call this.
 */
void compile_lock_leave(struct compile_lock *cl,
			enum compile_lock_status status);

static inline enum compile_lock_status
compile_lock_get_status(struct compile_lock *cl)
{
	return atomic_read(&cl->status);
}

static inline void compile_lock_set_status(struct compile_lock *cl,
					   enum compile_lock_status status)
{
	/* We need full barrier around status set. */
	smp_mb();
	atomic_set(&cl->status, status);
	smp_mb();
}

#endif
