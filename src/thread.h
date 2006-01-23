/*
 * Copyright (C) 2003, 2004, 2005 Robert Lougher <rob@lougher.demon.co.uk>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef CREATING
#include <pthread.h>
#include <setjmp.h>
#include <stdlib.h>

/* Thread states */

#define CREATING     0
#define STARTED      1
#define RUNNING      2
#define WAITING      3
#define SUSPENDED    5

/* thread priorities */

#define MIN_PRIORITY   1
#define NORM_PRIORITY  5
#define MAX_PRIORITY  10

/* Enable/Disable suspend modes */

#define SUSP_BLOCKING 1
#define SUSP_CRITICAL 2

typedef struct thread Thread;

typedef struct monitor {
    pthread_mutex_t lock;
    pthread_cond_t cv;
    Thread *owner;
    int count;
    int waiting;
    int notifying;
    int interrupting;
    uintptr_t entering;
    struct monitor *next;
    Object *obj;
} Monitor;

struct thread {
    char state;
    char interrupted;
    char interrupting;
    char suspend;
    char blocking;
    pthread_t tid;
    int id;
    ExecEnv *ee;
    void *stack_top;
    void *stack_base;
    Monitor *wait_mon;
    Thread *prev, *next;
};

extern Thread *threadSelf();
extern Thread *threadSelf0(Object *jThread);

extern void *getStackTop(Thread *thread);
extern void *getStackBase(Thread *thread);

extern void threadInterrupt(Thread *thread);
extern void threadSleep(Thread *thread, long long ms, int ns);
extern void threadYield(Thread *thread);

extern int threadIsAlive(Thread *thread);
extern int threadInterrupted(Thread *thread);
extern int threadIsInterrupted(Thread *thread);
extern int systemIdle(Thread *self);

extern void suspendAllThreads(Thread *thread);
extern void resumeAllThreads(Thread *thread);

extern void createVMThread(char *name, void (*start)(Thread*));

extern void disableSuspend0(Thread *thread, void *stack_top);
extern void enableSuspend(Thread *thread);
extern void fastEnableSuspend(Thread *thread);

#define disableSuspend(thread)          \
{                                       \
    sigjmp_buf *env;                    \
    env = alloca(sizeof(sigjmp_buf));   \
    sigsetjmp(*env, FALSE);             \
    disableSuspend0(thread, (void*)env);\
}

#define fastDisableSuspend(thread)      \
{                                       \
    thread->blocking = SUSP_CRITICAL;   \
    MBARRIER();                         \
}

typedef struct {
    pthread_mutex_t lock;
    pthread_cond_t cv;
} VMWaitLock;

typedef pthread_mutex_t VMLock;

#define initVMLock(lock) pthread_mutex_init(&lock, NULL)
#define initVMWaitLock(wait_lock) {            \
    pthread_mutex_init(&wait_lock.lock, NULL); \
    pthread_cond_init(&wait_lock.cv, NULL);    \
}

#define lockVMLock(lock, self) { \
    self->state = WAITING;       \
    pthread_mutex_lock(&lock);   \
    self->state = RUNNING;       \
}

#define tryLockVMLock(lock, self) \
    (pthread_mutex_trylock(&lock) == 0)

#define unlockVMLock(lock, self) if(self) pthread_mutex_unlock(&lock)

#define lockVMWaitLock(wait_lock, self) lockVMLock(wait_lock.lock, self)
#define unlockVMWaitLock(wait_lock, self) unlockVMLock(wait_lock.lock, self)

#define waitVMWaitLock(wait_lock, self) {                        \
    self->state = WAITING;                                       \
    pthread_cond_wait(&wait_lock.cv, &wait_lock.lock);           \
    self->state = RUNNING;                                       \
}

#define timedWaitVMWaitLock(wait_lock, self, ms) {               \
    struct timeval tv;                                           \
    struct timespec ts;                                          \
    gettimeofday(&tv, 0);                                        \
    ts.tv_sec = tv.tv_sec + ms/1000;                             \
    ts.tv_nsec = (tv.tv_usec + ((ms%1000)*1000))*1000;           \
    if(ts.tv_nsec > 999999999L) {                                \
        ts.tv_sec++;                                             \
        ts.tv_nsec -= 1000000000L;                               \
    }                                                            \
    self->state = WAITING;                                       \
    pthread_cond_timedwait(&wait_lock.cv, &wait_lock.lock, &ts); \
    self->state = RUNNING;                                       \
}

#define notifyVMWaitLock(wait_lock, self) pthread_cond_signal(&wait_lock.cv)
#define notifyAllVMWaitLock(wait_lock, self) pthread_cond_broadcast(&wait_lock.cv)
#endif

