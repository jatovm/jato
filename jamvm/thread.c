/*
 * Copyright (C) 2003, 2004, 2005, 2006 Robert Lougher <rob@lougher.org.uk>.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sched.h>

#include "jam.h"
#include "thread.h"
#include "lock.h"
#include "hash.h"

#ifdef TRACETHREAD
#define TRACE(fmt, ...) jam_printf(fmt, ## __VA_ARGS__)
#else
#define TRACE(fmt, ...)
#endif

#define HASHTABSZE 1<<4
HashTable thread_id_map;

/* Size of Java stack to use if no size is given */
static int dflt_stack_size;

/* Thread create/destroy lock and condvar */
static pthread_mutex_t lock;
static pthread_cond_t cv;

/* lock and condvar used by main thread to wait for
 * all non-daemon threads to die */
static pthread_mutex_t exit_lock;
static pthread_cond_t exit_cv;

/* Monitor for sleeping threads to do a timed-wait against */
static Monitor sleep_mon;

/* Thread specific key holding thread's Thread pntr */
static pthread_key_t threadKey;

/* Attributes for spawned threads */
static pthread_attr_t attributes;

/* The main thread info - head of the thread list */
static Thread main_thread;

/* Main thread ExecEnv */
static ExecEnv main_ee;

/* Various field offsets into java.lang.Thread &
   java.lang.VMThread - cached at startup and used
   in thread creation */
static int vmData_offset;
static int daemon_offset;
static int group_offset;
static int priority_offset;
static int name_offset;
static int vmthread_offset;
static int thread_offset;
static int threadId_offset;

/* Method table indexes of Thread.run method and
 * ThreadGroup.removeThread - cached at startup */
static int run_mtbl_idx;
static int rmveThrd_mtbl_idx;

static MethodBlock *addThread_mb;
static MethodBlock *init_mb;

/* Cached java.lang.Thread class */
static Class *thread_class;
static Class *vmthread_class;

/* Count of non-daemon threads still running in VM */
static int non_daemon_thrds = 0;

/* Counts used in the ThreadMXBean management API.  The
   counts start from one to include the main thread */
static int threads_count = 1;
static int peak_threads_count = 1;
static long long total_started_threads_count = 1;

/* Guards against threads starting while the "world is stopped" */
static int all_threads_suspended = FALSE;
static int threads_waiting_to_start = 0;

static int main_exited = FALSE;

/* Bitmap - used for generating unique thread ID's */
#define MAP_INC 32
static unsigned int *tidBitmap = NULL;
static int tidBitmapSize = 0;

/* Mark a threadID value as no longer used */
#define freeThreadID(n) tidBitmap[(n-1)>>5] &= ~(1<<((n-1)&0x1f))

/* Generate a new thread ID - assumes the thread queue
 * lock is held */

static int genThreadID() {
    int i = 0;

retry:
    for(; i < tidBitmapSize; i++) {
        if(tidBitmap[i] != 0xffffffff) {
            int n = ffs(~tidBitmap[i]);
            tidBitmap[i] |= 1 << (n-1);
            return (i<<5) + n;
        }
    }

    tidBitmap = (unsigned int *)sysRealloc(tidBitmap,
                       (tidBitmapSize + MAP_INC) * sizeof(unsigned int));
    memset(tidBitmap + tidBitmapSize, 0, MAP_INC * sizeof(unsigned int));
    tidBitmapSize += MAP_INC;
    goto retry;
}

int threadIsAlive(Thread *thread) {
    return thread->state != 0;
}

int threadInterrupted(Thread *thread) {
    int r = thread->interrupted;
    thread->interrupted = FALSE;
    return r;
}

int threadIsInterrupted(Thread *thread) {
    return thread->interrupted;
}

void threadSleep(Thread *thread, long long ms, int ns) {

   /* A sleep of zero should just yield, but a wait
      of zero is "forever".  Therefore wait for a tiny
      amount -- this should yield and we also get the
      interrupted checks. */
    if(ms == 0 &&  ns == 0)
        ns = 1;

    monitorLock(&sleep_mon, thread);
    monitorWait(&sleep_mon, thread, ms, ns);
    monitorUnlock(&sleep_mon, thread);
}

void threadYield(Thread *thread) {
    sched_yield();
}

void threadInterrupt(Thread *thread) {
    Monitor *mon;

    /* monitorWait sets wait_mon _before_ checking interrupted status.  Therefore,
       if wait_mon is null, interrupted status will be noticed.  This guards
       against a race-condition leading to an interrupt being missed.  The memory
       barrier ensures correct ordering on SMP systems. */
    thread->interrupted = TRUE;
    MBARRIER();

    if((mon = thread->wait_mon) != NULL && thread->wait_next != NULL) {
        int locked;
        thread->interrupting = TRUE;

        /* Ensure the thread has actually entered the wait
           (else the signal will be lost) */

        while(!(locked = !pthread_mutex_trylock(&mon->lock)) && mon->owner == NULL)
            sched_yield();

        pthread_cond_signal(&thread->wait_cv);

        if(locked)
            pthread_mutex_unlock(&mon->lock);
    }

    /* Handle the case where the thread is blocked in a system call.
       This will knock it out with an EINTR.  The suspend signal
       handler will just return (as in the user doing a kill), and do
       nothing otherwise. */

    pthread_kill(thread->tid, SIGUSR1);
}

void *getStackTop(Thread *thread) {
    return thread->stack_top;
}

void *getStackBase(Thread *thread) {
    return thread->stack_base;
}

Thread *threadSelf0(Object *vmThread) {
    return (Thread*)(INST_DATA(vmThread)[vmData_offset]);
}

Thread *threadSelf() {
    return (Thread*)pthread_getspecific(threadKey);
}

void setThreadSelf(Thread *thread) {
    pthread_setspecific(threadKey, thread);
    pthread_cond_init(&thread->wait_cv, NULL);
}

ExecEnv *getExecEnv() {
    return threadSelf()->ee;
}

char *getThreadStateString(Thread *thread) {
    switch(thread->state) {
        case CREATING:
        case STARTED:
            return "NEW";
        case RUNNING:
        case SUSPENDED:
            return "RUNNABLE";
        case WAITING:
            return "WAITING";
        case TIMED_WAITING:
            return "TIMED_WAITING";
        case BLOCKED:
            return "BLOCKED";
    }
}

int getThreadsCount() {
    return threads_count;
}

int getPeakThreadsCount() {
    return peak_threads_count;
}

long long getTotalStartedThreadsCount() {
    return total_started_threads_count;
}

void resetPeakThreadsCount() {
    /* Grab the thread lock to protect against
       concurrent update by threads starting/dying */
    pthread_mutex_lock(&lock);
    peak_threads_count = threads_count;
    pthread_mutex_unlock(&lock);
}

void initialiseJavaStack(ExecEnv *ee) {
   int stack_size = ee->stack_size ?
           (ee->stack_size > MIN_STACK ? ee->stack_size : MIN_STACK) : dflt_stack_size;
   char *stack = sysMalloc(stack_size);
   MethodBlock *mb = (MethodBlock *) stack;
   Frame *top = (Frame *) (mb+1);

   top->ostack = (uintptr_t*)(top+1);
   top->lvars = (uintptr_t*)top;
   mb->max_stack = 0;
   top->prev = NULL;
   top->mb = mb;

   ee->stack = stack;
   ee->last_frame = top;
   ee->stack_size = stack_size;
   ee->stack_end = stack + stack_size-STACK_RED_ZONE_SIZE;
}

long long javaThreadId(Thread *thread) {
    return *(long long*)&(INST_DATA(thread->ee->thread)[threadId_offset]);
}

Thread *findHashedThread(Thread *thread, long long id) {

#define FOUND(thread_id)
#define DELETED ((void*)-1)
#define PREPARE(thread_id) thread
#define SCAVENGE(ptr) ptr == DELETED
#define HASH(thread_id) (int)thread_id
#define COMPARE(thread_id, ptr, hash1, hash2) ptr != DELETED && \
                                              (hash1 == hash2 && thread_id == javaThreadId(ptr))

    Thread *ptr;

    /* Add if absent, scavenge, locked */
    findHashEntry(thread_id_map, id, ptr, (thread != NULL), TRUE, TRUE);

    return ptr;
}

void addThreadToHash(Thread *thread) {
    findHashedThread(thread, javaThreadId(thread));
}

Thread *findThreadById(long long id) {
    return findHashedThread(NULL, id);
}

void deleteThreadFromHash(Thread *thread) {

#undef HASH
#undef COMPARE
#define HASH(ptr) (int)javaThreadId(ptr)
#define COMPARE(ptr1, ptr2, hash1, hash2) ptr1 == ptr2

    deleteHashEntry(thread_id_map, thread, TRUE);
}

Object *initJavaThread(Thread *thread, char is_daemon, char *name) {
    Object *vmthread, *jlthread, *thread_name = NULL;

    /* Create the java.lang.Thread object and the VMThread */
    if((vmthread = allocObject(vmthread_class)) == NULL ||
       (jlthread = allocObject(thread_class)) == NULL)
        return NULL;

    thread->ee->thread = jlthread;
    INST_DATA(vmthread)[vmData_offset] = (uintptr_t)thread;
    INST_DATA(vmthread)[thread_offset] = (uintptr_t)jlthread;

    /* Create the string for the thread name.  If null is specified
       the initialiser method will generate a name of the form Thread-X */
    if(name != NULL && (thread_name = Cstr2String(name)) == NULL)
        return NULL;

    /* Call the initialiser method -- this is for use by threads
       created or attached by the VM "outside" of Java */
    executeMethod(jlthread, init_mb, vmthread, thread_name, NORM_PRIORITY, is_daemon);

    /* Add thread to thread ID map hash table. */
    addThreadToHash(thread);

    return jlthread;
}

void initThread(Thread *thread, char is_daemon, void *stack_base) {

    /* Create the thread stack and store the thread structure in
       thread-specific memory */
    initialiseJavaStack(thread->ee);
    setThreadSelf(thread);

    /* Record the thread's stack base */
    thread->stack_base = stack_base;

    /* Grab thread list lock.  This also stops suspension */
    pthread_mutex_lock(&lock);

    /* If all threads are suspended (i.e. GC in progress) we cannot start
       until the threads are resumed.  Record we're waiting and wait until
       the world is restarted... */

    threads_waiting_to_start++;

    while(all_threads_suspended)
        pthread_cond_wait(&cv, &lock);

    threads_waiting_to_start--;

    /* add to thread list... After this point (once we release the lock)
       we are suspendable */
    if((thread->next = main_thread.next))
        main_thread.next->prev = thread;
    thread->prev = &main_thread;
    main_thread.next = thread;

    /* Keep track of threads counts */
    if(++threads_count > peak_threads_count)
        peak_threads_count = threads_count;
    total_started_threads_count++;

    /* If we're not a deamon thread the main thread must
       wait until we exit */
    if(!is_daemon)
        non_daemon_thrds++;

    /* Get a thread ID (used in thin locking) and
       record we're now running */
    thread->id = genThreadID();
    thread->state = RUNNING;

    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&lock);
}

Thread *attachThread(char *name, char is_daemon, void *stack_base, Thread *thread, Object *group) {
    Object *java_thread;

    /* Create the ExecEnv for the thread */
    ExecEnv *ee = sysMalloc(sizeof(ExecEnv));
    memset(ee, 0, sizeof(ExecEnv));

    thread->tid = pthread_self();
    thread->ee = ee;

    /* Complete initialisation of the thread structure, create the thread
       stack and add the thread to the thread list */
    initThread(thread, is_daemon, stack_base);

    /* Initialise the Java-level thread objects representing this thread */
    if((java_thread = initJavaThread(thread, is_daemon, name)) == NULL)
        return NULL;

    /* Initialiser doesn't handle the thread group */
    INST_DATA(java_thread)[group_offset] = (uintptr_t)group;
    executeMethod(group, addThread_mb, java_thread);

    /* We're now attached to the VM...*/
    TRACE("Thread 0x%x id: %d attached\n", thread, thread->id);
    return thread;
}

void detachThread(Thread *thread) {
    Object *group, *excep;
    ExecEnv *ee = thread->ee;
    Object *jThread = ee->thread;
    Object *vmthread = (Object*)INST_DATA(jThread)[vmthread_offset];

    /* Get the thread's group */
    group = (Object *)INST_DATA(jThread)[group_offset];

    /* If there's an uncaught exception, call uncaughtException on the thread's
       exception handler, or the thread's group if this is unset */
    if((excep = exceptionOccured())) {
        FieldBlock *fb = findField(thread_class, "exceptionHandler",
                                                 "Ljava/lang/Thread$UncaughtExceptionHandler;");
        Object *thread_handler = fb == NULL ? NULL : (Object *)INST_DATA(jThread)[fb->offset];
        Object *handler = thread_handler == NULL ? group : thread_handler;

        MethodBlock *uncaught_exp = lookupMethod(handler->class, "uncaughtException",
                                                 "(Ljava/lang/Thread;Ljava/lang/Throwable;)V");

        if(uncaught_exp) {
            clearException();
            executeMethod(handler, uncaught_exp, jThread, excep);
        } else
            printException();
    }

    /* remove thread from thread group */
    executeMethod(group, (CLASS_CB(group->class))->method_table[rmveThrd_mtbl_idx], jThread);

    /* set VMThread ref in Thread object to null - operations after this
       point will result in an IllegalThreadStateException */
    INST_DATA(jThread)[vmthread_offset] = 0;

    /* Disable suspend to protect lock operation */
    disableSuspend0(thread, &excep);

    /* Grab global lock, and update thread structures protected by
       it (thread list, thread ID and number of daemon threads) */
    pthread_mutex_lock(&lock);

    /* remove from thread list... */
    if((thread->prev->next = thread->next))
        thread->next->prev = thread->prev;

    /* One less live thread */
    threads_count--;

    /* Recycle the thread's thread ID */
    freeThreadID(thread->id);

    /* Handle daemon thread status */
    if(!INST_DATA(jThread)[daemon_offset])
        non_daemon_thrds--;

    pthread_mutex_unlock(&lock);

    /* notify any threads waiting on VMThread object -
       these are joining this thread */
    objectLock(vmthread);
    objectNotifyAll(vmthread);
    objectUnlock(vmthread);

    /* It is safe to free the thread's ExecEnv and stack now as these are
       only used within the thread.  It is _not_ safe to free the native
       thread structure as another thread may be concurrently accessing it.
       However, they must have a reference to the VMThread -- therefore, it
       is safe to free during GC when the VMThread is determined to be no
       longer reachable. */
    free(ee->stack);
    free(ee);

    /* If no more daemon threads notify the main thread (which
       may be waiting to exit VM).  Note, this is not protected
       by lock, but main thread checks again */

    if(non_daemon_thrds == 0) {
        /* No need to bother with disabling suspension
         * around lock, as we're no longer on thread list */
        pthread_mutex_lock(&exit_lock);
        pthread_cond_signal(&exit_cv);
        pthread_mutex_unlock(&exit_lock);
    }

    /* Remove thread from the ID map hash table */
    deleteThreadFromHash(thread);

    TRACE("Thread 0x%x id: %d detached from VM\n", thread, thread->id);
}

void *threadStart(void *arg) {
    Thread *thread = (Thread *)arg;
    ExecEnv *ee = thread->ee;
    Object *jThread = ee->thread;
    ClassBlock *cb = CLASS_CB(jThread->class);
    MethodBlock *run = cb->method_table[run_mtbl_idx];

    /* Complete initialisation of the thread structure, create the thread
       stack and add the thread to the thread list */
    initThread(thread, INST_DATA(jThread)[daemon_offset], &thread);

    /* Add thread to thread ID map hash table. */
    addThreadToHash(thread);

    /* Execute the thread's run method */
    executeMethod(jThread, run);

    /* Run has completed.  Detach the thread from the VM and exit */
    detachThread(thread);

    TRACE("Thread 0x%x id: %d exited\n", thread, thread->id);
    return NULL;
}

void createJavaThread(Object *jThread, long long stack_size) {
    ExecEnv *ee;
    Thread *thread;
    Thread *self = threadSelf();
    Object *vmthread = allocObject(vmthread_class);

    if(vmthread == NULL)
        return;

    disableSuspend(self);

    pthread_mutex_lock(&lock);
    if(INST_DATA(jThread)[vmthread_offset]) {
        pthread_mutex_unlock(&lock);
        enableSuspend(self);
        signalException("java/lang/IllegalThreadStateException", "thread already started");
        return;
    }

    ee = (ExecEnv*)sysMalloc(sizeof(ExecEnv));
    thread = (Thread*)sysMalloc(sizeof(Thread));
    memset(ee, 0, sizeof(ExecEnv));
    memset(thread, 0, sizeof(Thread));

    thread->ee = ee;
    ee->thread = jThread;
    ee->stack_size = stack_size;

    INST_DATA(vmthread)[vmData_offset] = (uintptr_t)thread;
    INST_DATA(vmthread)[thread_offset] = (uintptr_t)jThread;
    INST_DATA(jThread)[vmthread_offset] = (uintptr_t)vmthread;

    pthread_mutex_unlock(&lock);

    if(pthread_create(&thread->tid, &attributes, threadStart, thread)) {
        INST_DATA(jThread)[vmthread_offset] = 0;
        free(ee);
        free(thread);
        signalException("java/lang/OutOfMemoryError", "can't create thread");
        return;
    }

    pthread_mutex_lock(&lock);

    /* Wait for thread to start */
    while(thread->state == 0)
        pthread_cond_wait(&cv, &lock);

    pthread_mutex_unlock(&lock);
    enableSuspend(self);
}

static void initialiseSignals();

Thread *attachJNIThread(char *name, char is_daemon, Object *group) {
    Thread *thread = sysMalloc(sizeof(Thread));
    void *stack_base = nativeStackBase();

    /* If no group is given add it to the main group */
    if(group == NULL)
        group = (Object*)INST_DATA(main_ee.thread)[group_offset];

    /* Initialise internal thread structure */
    memset(thread, 0, sizeof(Thread));

    /* Externally created threads will not inherit signal state */
    initialiseSignals();

    /* Initialise the thread and add it to the VM thread list */
    return attachThread(name, is_daemon, stack_base, thread, group);
}

void detachJNIThread(Thread *thread) {
    /* The JNI spec says we should release all locks held by this thread.
       We don't do this (yet), and only remove the thread from the VM. */
    detachThread(thread);
}

static void *shell(void *args) {
    void *start = ((void**)args)[1];
    Thread *self = ((Thread**)args)[2];

    if(main_exited)
        return NULL;

    /* VM helper threads should be added to the system group, but this doesn't
       exist.  As the root group is main, we add it to that for now... */
    attachThread(((char**)args)[0], TRUE, &self, self,
                 (Object*)INST_DATA(main_ee.thread)[group_offset]);

    free(args);
    (*(void(*)(Thread*))start)(self);
    return NULL;
}

void createVMThread(char *name, void (*start)(Thread*)) {
    Thread *thread = (Thread*)sysMalloc(sizeof(Thread));
    void **args = sysMalloc(3 * sizeof(void*));
    pthread_t tid;

    args[0] = name;
    args[1] = start;
    args[2] = thread;

    memset(thread, 0, sizeof(Thread));
    pthread_create(&tid, &attributes, shell, args);

    /* Wait for thread to start */

    pthread_mutex_lock(&lock);
    while(thread->state == 0)
        pthread_cond_wait(&cv, &lock);
    pthread_mutex_unlock(&lock);
}

void suspendThread(Thread *thread) {
    thread->suspend = TRUE;
    MBARRIER();

    if(!thread->blocking) {
        TRACE("Sending suspend signal to thread 0x%x id: %d\n", thread, thread->id);
        pthread_kill(thread->tid, SIGUSR1);
    }

    while(thread->blocking != SUSP_BLOCKING && thread->state != SUSPENDED) {
        TRACE("Waiting for thread 0x%x id: %d to suspend\n", thread, thread->id);
        sched_yield();
    }
}

void resumeThread(Thread *thread) {
    thread->suspend = FALSE;
    MBARRIER();

    if(!thread->blocking) {
        TRACE("Sending resume signal to thread 0x%x id: %d\n", thread, thread->id);
        pthread_kill(thread->tid, SIGUSR1);
    }

    while(thread->state == SUSPENDED) {
        TRACE("Waiting for thread 0x%x id: %d to resume\n", thread, thread->id);
        sched_yield();
    }
}

void suspendAllThreads(Thread *self) {
    Thread *thread;

    TRACE("Thread 0x%x id: %d is suspending all threads\n", self, self->id);
    pthread_mutex_lock(&lock);

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        if(thread == self)
            continue;

        thread->suspend = TRUE;
        MBARRIER();

        if(!thread->blocking) {
            TRACE("Sending suspend signal to thread 0x%x id: %d\n", thread, thread->id);
            pthread_kill(thread->tid, SIGUSR1);
        }
    }

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        if(thread == self)
            continue;

        while(thread->blocking != SUSP_BLOCKING && thread->state != SUSPENDED) {
            TRACE("Waiting for thread 0x%x id: %d to suspend\n", thread, thread->id);
            sched_yield();
        }
    }

    all_threads_suspended = TRUE;

    TRACE("All threads suspended...\n");
    pthread_mutex_unlock(&lock);
}

void resumeAllThreads(Thread *self) {
    Thread *thread;

    TRACE("Thread 0x%x id: %d is resuming all threads\n", self, self->id);
    pthread_mutex_lock(&lock);

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        if(thread == self)
            continue;

        thread->suspend = FALSE;
        MBARRIER();

        if(!thread->blocking) {
            TRACE("Sending resume signal to thread 0x%x id: %d\n", thread, thread->id);
            pthread_kill(thread->tid, SIGUSR1);
        }
    }

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        while(thread->state == SUSPENDED) {
            TRACE("Waiting for thread 0x%x id: %d to resume\n", thread, thread->id);
            sched_yield();
        }
    }

    all_threads_suspended = FALSE;
    if(threads_waiting_to_start) {
        TRACE("%d threads waiting to start...\n", threads_waiting_to_start);
	    pthread_cond_broadcast(&cv);
    }

    TRACE("All threads resumed...\n");
    pthread_mutex_unlock(&lock);
}

static void suspendLoop(Thread *thread) {
    char old_state = thread->state;
    sigjmp_buf env;
    sigset_t mask;

    sigsetjmp(env, FALSE);

    thread->stack_top = &env;
    thread->state = SUSPENDED;
    MBARRIER();

    sigfillset(&mask);
    sigdelset(&mask, SIGUSR1);
    sigdelset(&mask, SIGTERM);

    while(thread->suspend && !thread->blocking)
        sigsuspend(&mask);

    thread->state = old_state;
    MBARRIER();
}

static void suspendHandler(int sig) {
    Thread *thread = threadSelf();
    suspendLoop(thread);
}

/* The "slow" disable.  Normally used when a thread enters
   a blocking code section, such as waiting on a lock.  */

void disableSuspend0(Thread *thread, void *stack_top) {
    sigset_t mask;

    thread->stack_top = stack_top;
    thread->blocking = SUSP_BLOCKING;
    MBARRIER();

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);
}

void enableSuspend(Thread *thread) {
    sigset_t mask;

    thread->blocking = FALSE;
    MBARRIER();

    if(thread->suspend) {
        TRACE("Thread 0x%x id: %d is self suspending\n", thread, thread->id);
        suspendLoop(thread);
        TRACE("Thread 0x%x id: %d resumed\n", thread, thread->id);
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
}

/* Fast disable/enable suspend doesn't modify the signal mask
   or save the thread context.  It's used in critical code
   sections where the thread cannot be suspended (such as
   modifying the loaded classes hash table).  Thread supension
   blocks until the thread self-suspends.  */

void fastEnableSuspend(Thread *thread) {

    thread->blocking = FALSE;
    MBARRIER();

    if(thread->suspend) {
        sigset_t mask;

        sigemptyset(&mask);
        sigaddset(&mask, SIGUSR1);
        pthread_sigmask(SIG_BLOCK, &mask, NULL);

        TRACE("Thread 0x%x id: %d is fast self suspending\n", thread, thread->id);
        suspendLoop(thread);
        TRACE("Thread 0x%x id: %d resumed\n", thread, thread->id);

        pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
    }
}

void dumpThreadsLoop(Thread *self) {
    Thread *thread;
    sigset_t mask;
    int sig;

    sigemptyset(&mask);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGINT);

    disableSuspend0(self, &self);
    for(;;) {
        sigwait(&mask, &sig);

        /* If it was an interrupt (e.g. Ctrl-C) terminate the VM */
        if(sig == SIGINT)
            exitVM(0);

        /* It must be a SIGQUIT.  Do a thread dump */

        suspendAllThreads(self);
        jam_printf("\n------ JamVM version %s Full Thread Dump -------\n", VERSION);

        for(thread = &main_thread; thread != NULL; thread = thread->next) {
            uintptr_t *thr_data = INST_DATA(thread->ee->thread);
            char *name = String2Cstr((Object*)thr_data[name_offset]);
            int priority = thr_data[priority_offset];
            int daemon = thr_data[daemon_offset];
            Frame *last = thread->ee->last_frame;

            jam_printf("\n\"%s\"%s %p priority: %d tid: %p id: %d state: %s (%d)\n",
                  name, daemon ? " (daemon)" : "", thread, priority, thread->tid,
                  thread->id, getThreadStateString(thread), thread->state);
            free(name);

            while(last->prev != NULL) {
                for(; last->mb != NULL; last = last->prev) {
                    MethodBlock *mb = last->mb;
                    ClassBlock *cb = CLASS_CB(mb->class);
                    char *dot_name = slash2dots(cb->name);

                    jam_printf("\tat %s.%s(", dot_name, mb->name);
                    free(dot_name);

                    if(mb->access_flags & ACC_NATIVE)
                        jam_printf("Native method");
                    else
                        if(cb->source_file_name == NULL)
                            jam_printf("Unknown source");
                        else {
                            int line = mapPC2LineNo(mb, last->last_pc);
                            jam_printf("%s", cb->source_file_name);
                            if(line != -1)
                                jam_printf(":%d", line);
                        }
                    jam_printf(")\n");
                }
                last = last->prev;
            }
        }
        resumeAllThreads(self);
    }
}

static void initialiseSignals() {
    struct sigaction act;
    sigset_t mask;

    act.sa_handler = suspendHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGUSR1, &act, NULL);

    sigemptyset(&mask);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGPIPE);
    sigprocmask(SIG_BLOCK, &mask, NULL);
}

/* garbage collection support */

extern void scanThread(Thread *thread);

void scanThreads() {
    Thread *thread;

    pthread_mutex_lock(&lock);
    for(thread = &main_thread; thread != NULL; thread = thread->next)
        scanThread(thread);
    pthread_mutex_unlock(&lock);
}

int systemIdle(Thread *self) {
    Thread *thread;

    for(thread = &main_thread; thread != NULL; thread = thread->next)
        if(thread != self && thread->state < WAITING)
            return FALSE;

    return TRUE;
}

void exitVM(int status) {
    main_exited = TRUE;

    /* Execute System.exit() to run any registered shutdown hooks.
       In the unlikely event that System.exit() can't be found, or
       it returns, fall through and exit. */

    if(!VMInitialising()) {
        Class *system = findSystemClass("java/lang/System");
        if(system) {
            MethodBlock *exit = findMethod(system, "exit", "(I)V");
            if(exit)
                executeStaticMethod(system, exit, status);
        }
    }

    jamvm_exit(status);
}

void mainThreadWaitToExitVM() {
    Thread *self = threadSelf();
    TRACE("Waiting for %d non-daemon threads to exit\n", non_daemon_thrds);

    disableSuspend(self);
    pthread_mutex_lock(&exit_lock);

    self->state = WAITING;
    while(non_daemon_thrds)
        pthread_cond_wait(&exit_cv, &exit_lock);

    pthread_mutex_unlock(&exit_lock);
    enableSuspend(self);
}

void mainThreadSetContextClassLoader(Object *loader) {
    FieldBlock *fb = findField(thread_class, "contextClassLoader", "Ljava/lang/ClassLoader;");
    if(fb != NULL)
        INST_DATA(main_ee.thread)[fb->offset] = (uintptr_t)loader;
}

void initialiseMainThread(InitArgs *args) {
    size_t size;
    Object *java_thread;
    Class *thrdGrp_class;
    MethodBlock *run, *remove_thread;
    FieldBlock *group, *priority, *root, *threadId;
    FieldBlock *vmData, *vmThread, *thread, *daemon, *name;

    /* Set the default size of the Java stack for each _new_ thread */
    dflt_stack_size = args->java_stack;

    /* Initialise internal locks and pthread state */
    pthread_key_create(&threadKey, NULL);

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    pthread_mutex_init(&exit_lock, NULL);
    pthread_cond_init(&exit_cv, NULL);

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

    /* Ensure the thread stacks are at least 2 megabytes in size */
    pthread_attr_getstacksize(&attributes, &size);
    if(size < 2*MB)
        pthread_attr_setstacksize(&attributes, 2*MB);

    monitorInit(&sleep_mon);
    initHashTable(thread_id_map, HASHTABSZE, TRUE);

    /* We need to cache field and method offsets to create and initialise
       threads.  However, the class loading component requires a valid
       thread and ExecEnv.  To get round this we initialise the main thread
       in two parts.  First part is sufficient to _load_ classes, but it
       is still not fully setup so we don't allow initialisers to run */

    main_thread.stack_base = args->main_stack_base;
    main_thread.tid = pthread_self();
    main_thread.id = genThreadID();
    main_thread.state = RUNNING;
    main_thread.ee = &main_ee;

    initialiseJavaStack(&main_ee);
    setThreadSelf(&main_thread);

    /* As we're initialising, VM will abort if Thread can't be found */
    thread_class = findSystemClass0("java/lang/Thread");

    /* Register class reference for compaction threading */
    registerStaticClassRef(&thread_class);

    vmThread = findField(thread_class, "vmThread", "Ljava/lang/VMThread;");
    daemon = findField(thread_class, "daemon", "Z");
    name = findField(thread_class, "name", "Ljava/lang/String;");
    group = findField(thread_class, "group", "Ljava/lang/ThreadGroup;");
    priority = findField(thread_class, "priority", "I");
    threadId = findField(thread_class, "threadId", "J");

    init_mb = findMethod(thread_class, "<init>",
                         "(Ljava/lang/VMThread;Ljava/lang/String;IZ)V");
    run = findMethod(thread_class, "run", "()V");

    vmthread_class = findSystemClass0("java/lang/VMThread");
    CLASS_CB(vmthread_class)->flags |= VMTHREAD;

    /* Register class reference for compaction threading */
    registerStaticClassRef(&vmthread_class);

    thread = findField(vmthread_class, "thread", "Ljava/lang/Thread;");
    vmData = findField(vmthread_class, "vmData", "I");

    /* findField and findMethod do not throw an exception... */
    if((init_mb == NULL) || (vmData == NULL) || (run == NULL) || (daemon == NULL) ||
       (name == NULL) || (group == NULL) || (priority == NULL) || (vmThread == NULL) ||
       (thread == NULL) || (threadId == NULL))
        goto error;

    vmthread_offset = vmThread->offset;
    thread_offset = thread->offset;
    vmData_offset = vmData->offset;
    daemon_offset = daemon->offset;
    group_offset = group->offset;
    priority_offset = priority->offset;
    threadId_offset = threadId->offset;
    name_offset = name->offset;
    run_mtbl_idx = run->method_table_index;

    /* Initialise the Java-level thread objects representing this thread */
    java_thread = initJavaThread(&main_thread, FALSE, "main");

    /* Main thread is now sufficiently setup to be able to run the thread group
       initialiser.  This is essential to create the root thread group */
    thrdGrp_class = findSystemClass("java/lang/ThreadGroup");

    root = findField(thrdGrp_class, "root", "Ljava/lang/ThreadGroup;");
    addThread_mb = findMethod(thrdGrp_class, "addThread", "(Ljava/lang/Thread;)V");
    remove_thread = findMethod(thrdGrp_class, "removeThread", "(Ljava/lang/Thread;)V");

    /* findField and findMethod do not throw an exception... */
    if((root == NULL) || (addThread_mb == NULL) || (remove_thread == NULL))
        goto error;

    rmveThrd_mtbl_idx = remove_thread->method_table_index;

    /* Add the main thread to the root thread group */
    INST_DATA(java_thread)[group_offset] = root->static_value;
    executeMethod(((Object*)root->static_value), addThread_mb, java_thread);

    /* Setup signal handling.  This will be inherited by all
       threads created within Java */
    initialiseSignals();

    /* Create the signal handler thread.  It is responsible for
       catching and handling SIGQUIT (thread dump) and SIGINT
       (user-termination of the VM, e.g. via Ctrl-C).  Note it
       must be a valid Java-level thread as it needs to run the
       shutdown hooks in the event of user-termination */
    createVMThread("Signal Handler", dumpThreadsLoop);

    return;

error:
    jam_fprintf(stderr, "Error initialising VM (initialiseMainThread)\n");
    exitVM(1);
}
