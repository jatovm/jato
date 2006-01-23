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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sched.h>

#include "jam.h"
#include "thread.h"
#include "lock.h"

#ifdef TRACETHREAD
#define TRACE(x) printf x
#else
#define TRACE(x)
#endif

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

/* Method table indexes of Thread.run method and
 * ThreadGroup.removeThread - cached at startup */
static int run_mtbl_idx;
static int rmveThrd_mtbl_idx;

/* Cached java.lang.Thread class */
static Class *thread_class;
static Class *vmthread_class;

/* Count of non-daemon threads still running in VM */
static int non_daemon_thrds = 0;

static int main_exited = FALSE;

/* Bitmap - used for generating unique thread ID's */
#define MAP_INC 32
static unsigned int *tidBitmap;
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

#define INTERRUPT_TRIES 5

void threadInterrupt(Thread *thread) {
    Monitor *mon = thread->wait_mon;

    thread->interrupted = TRUE;

    if(mon) {
        Thread *self = threadSelf();
        char owner = (mon->owner == self);
        if(!owner) {
            int i;

            /* Another thread may be holding the monitor -
             * if we can't get it after a couple attempts give-up
             * to avoid deadlock */
            for(i = 0; i < INTERRUPT_TRIES; i++) {
                if(!pthread_mutex_trylock(&mon->lock))
                    goto got_lock;
                sched_yield();
                if(thread->wait_mon != mon)
                    return;
            }
            return;
        }
got_lock:
        if((thread->wait_mon == mon) && thread->interrupted && !thread->interrupting &&
                  ((mon->notifying + mon->interrupting) < mon->waiting)) {
            thread->interrupting = TRUE;
            mon->interrupting++;
            pthread_cond_broadcast(&mon->cv);
        }
        if(!owner)
            pthread_mutex_unlock(&mon->lock);
    }
}

void *getStackTop(Thread *thread) {
    return thread->stack_top;
}

void *getStackBase(Thread *thread) {
    return thread->stack_base;
}

Thread *threadSelf0(Object *jThread) {
    return (Thread*)(INST_DATA(jThread)[vmData_offset]);
}

Thread *threadSelf() {
    return (Thread*)pthread_getspecific(threadKey);
}

void setThreadSelf(Thread *thread) {
   pthread_setspecific(threadKey, thread);
}

ExecEnv *getExecEnv() {
    return threadSelf()->ee;
}

void initialiseJavaStack(ExecEnv *ee) {
   int stack_size = ee->stack_size ?
           (ee->stack_size > MIN_STACK ? ee->stack_size : MIN_STACK) : dflt_stack_size;
   char *stack = sysMalloc(stack_size);
   MethodBlock *mb = (MethodBlock *) stack;
   Frame *top = (Frame *) (mb+1);

   mb->max_stack = 0;
   top->mb = mb;
   top->ostack = (uintptr_t*)(top+1);
   top->prev = 0;

   ee->stack = stack;
   ee->last_frame = top;
   ee->stack_size = stack_size;
   ee->stack_end = stack + stack_size-STACK_RED_ZONE_SIZE;
}

void *threadStart(void *arg) {
    Thread *thread = (Thread *)arg;
    ExecEnv *ee = thread->ee;
    Object *jThread = ee->thread;
    ClassBlock *cb = CLASS_CB(jThread->class);
    MethodBlock *run = cb->method_table[run_mtbl_idx];
    Object *vmthread = (Object*)INST_DATA(jThread)[vmthread_offset];
    Object *group, *excep;

    initialiseJavaStack(ee);
    setThreadSelf(thread);

    /* Need to disable suspension as we'll most likely
     * be waiting on lock when we're added to the thread
     * list, and now liable for suspension */

    thread->stack_base = &group;
    disableSuspend0(thread, &group);

    pthread_mutex_lock(&lock);
    thread->id = genThreadID();

    TRACE(("Thread 0x%x id: %d started\n", thread, thread->id));

    thread->state = STARTED;
    pthread_cond_broadcast(&cv);

    while(thread->state != RUNNING)
        pthread_cond_wait(&cv, &lock);
    pthread_mutex_unlock(&lock);

    /* Execute the thread's run() method... */
    enableSuspend(thread);
    executeMethod(jThread, run);

    /* Call thread group's uncaughtException if exception
     * is of type java.lang.Throwable */

    group = (Object *)INST_DATA(jThread)[group_offset];
    if((excep = exceptionOccured())) {
        Class *throwable;
        MethodBlock *uncaught_exp;
       
        clearException();
        throwable = findSystemClass0("java/lang/Throwable");
        if(throwable && isInstanceOf(throwable, excep->class)
                     && (uncaught_exp = lookupMethod(group->class, "uncaughtException",
                                                      "(Ljava/lang/Thread;Ljava/lang/Throwable;)V")))
            executeMethod(group, uncaught_exp, jThread, excep);
        else {
            setException(excep);
            printException();
        }
    }

    /* remove thread from thread group */
    executeMethod(group, (CLASS_CB(group->class))->method_table[rmveThrd_mtbl_idx], jThread);

    /* set VMThread ref in Thread object to null - operations after this
       point will result in an IllegalThreadStateException */
    INST_DATA(jThread)[vmthread_offset] = 0;

    /* Disable suspend to protect lock operation */
    disableSuspend0(thread, &group);

    /* Grab global lock, and update thread structures protected by
       it (thread list, thread ID and number of daemon threads) */
    pthread_mutex_lock(&lock);

    /* remove from thread list... */
    if((thread->prev->next = thread->next))
        thread->next->prev = thread->prev;

    /* Recycle the threads thread ID */
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

    /* Free structures */
    INST_DATA(vmthread)[vmData_offset] = 0;
    free(thread);
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

    TRACE(("Thread 0x%x id: %d exited\n", thread, thread->id));
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
        enableSuspend(self);
        signalException("java/lang/OutOfMemoryError", "can't create thread");
        return;
    }

    pthread_mutex_lock(&lock);
    while(thread->state != STARTED)
        pthread_cond_wait(&cv, &lock);

    /* add to thread list... */

    if((thread->next = main_thread.next))
        main_thread.next->prev = thread;
    thread->prev = &main_thread;
    main_thread.next = thread;

    if(!INST_DATA(jThread)[daemon_offset])
        non_daemon_thrds++;

    /* Thread is now on the thread queue and liable for suspension.
       BUT there could have been a suspension before the thread
       started...  If so, we're suspended, so don't let it continue
       until we're resumed */

    pthread_mutex_unlock(&lock);
    enableSuspend(self);

    disableSuspend0(self, self->stack_top);
    pthread_mutex_lock(&lock);
    thread->state = RUNNING;
    pthread_cond_broadcast(&cv);

    pthread_mutex_unlock(&lock);
    enableSuspend(self);
}

Thread *attachThread(char *name, char is_daemon, void *stack_base, Thread *thread) {
    ExecEnv *ee;
    Object *vmthread;

    ee = (ExecEnv*)sysMalloc(sizeof(ExecEnv));
    memset(ee, 0, sizeof(ExecEnv));

    thread->tid = pthread_self();
    thread->stack_base = stack_base;
    thread->ee = ee;

    initialiseJavaStack(ee);
    setThreadSelf(thread);

    vmthread = allocObject(vmthread_class);
    ee->thread = allocObject(thread_class);

    INST_DATA(vmthread)[vmData_offset] = (uintptr_t)thread;
    INST_DATA(vmthread)[thread_offset] = (uintptr_t)ee->thread;

    INST_DATA(ee->thread)[daemon_offset] = is_daemon;
    INST_DATA(ee->thread)[name_offset] = (uintptr_t)Cstr2String(name);
    INST_DATA(ee->thread)[group_offset] = INST_DATA(main_ee.thread)[group_offset];
    INST_DATA(ee->thread)[priority_offset] = NORM_PRIORITY;
    INST_DATA(ee->thread)[vmthread_offset] = (uintptr_t)vmthread;

    /* add to thread list... */

    pthread_mutex_lock(&lock);
    if((thread->next = main_thread.next))
        main_thread.next->prev = thread;
    thread->prev = &main_thread;
    main_thread.next = thread;

    if(!is_daemon)
        non_daemon_thrds++;

    thread->id = genThreadID();
    thread->state = RUNNING;

    pthread_cond_broadcast(&cv);
    pthread_mutex_unlock(&lock);

    TRACE(("Thread 0x%x id: %d attached\n", thread, thread->id));
    return thread;
}

static void *shell(void *args) {
    void *start = ((void**)args)[1];
    Thread *self = ((Thread**)args)[2];

    if(main_exited)
        return NULL;

    attachThread(((char**)args)[0], TRUE, &self, self);

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

void suspendAllThreads(Thread *self) {
    Thread *thread;

    TRACE(("Thread 0x%x id: %d is suspending all threads\n", self, self->id));
    pthread_mutex_lock(&lock);

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        if(thread == self)
            continue;

        thread->suspend = TRUE;
        MBARRIER();

        if(!thread->blocking) {
            TRACE(("Sending suspend signal to thread 0x%x id: %d\n", thread, thread->id));
            pthread_kill(thread->tid, SIGUSR1);
        }
    }

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        if(thread == self)
            continue;

        while(thread->blocking != SUSP_BLOCKING && thread->state != SUSPENDED) {
            TRACE(("Waiting for thread 0x%x id: %d to suspend\n", thread, thread->id));
            sched_yield();
        }
    }

    TRACE(("All threads suspended...\n"));
    pthread_mutex_unlock(&lock);
}

void resumeAllThreads(Thread *self) {
    Thread *thread;

    TRACE(("Thread 0x%x id: %d is resuming all threads\n", self, self->id));
    pthread_mutex_lock(&lock);

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        if(thread == self)
            continue;

        thread->suspend = FALSE;
        MBARRIER();

        if(!thread->blocking) {
            TRACE(("Sending resume signal to thread 0x%x id: %d\n", thread, thread->id));
            pthread_kill(thread->tid, SIGUSR1);
        }
    }

    for(thread = &main_thread; thread != NULL; thread = thread->next) {
        while(thread->state == SUSPENDED) {
            TRACE(("Waiting for thread 0x%x id: %d to resume\n", thread, thread->id));
            sched_yield();
        }
    }

    TRACE(("All threads resumed...\n"));
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
        TRACE(("Thread 0x%x id: %d is self suspending\n", thread, thread->id));
        suspendLoop(thread);
        TRACE(("Thread 0x%x id: %d resumed\n", thread, thread->id));
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

        TRACE(("Thread 0x%x id: %d is fast self suspending\n", thread, thread->id));
        suspendLoop(thread);
        TRACE(("Thread 0x%x id: %d resumed\n", thread, thread->id));

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

        if(sig == SIGINT)
            exitVM(0);

        suspendAllThreads(self);
        printf("\n------ JamVM version %s Full Thread Dump -------\n", VERSION);

        for(thread = &main_thread; thread != NULL; thread = thread->next) {
            uintptr_t *thr_data = INST_DATA(thread->ee->thread);
            char *name = String2Cstr((Object*)thr_data[name_offset]);
            int priority = thr_data[priority_offset];
            int daemon = thr_data[daemon_offset];
            Frame *last = thread->ee->last_frame;

            printf("\n\"%s\"%s priority: %d tid: %p id: %d state: %d\n",
                  name, daemon ? " (daemon)" : "", priority,
                  thread->tid, thread->id, thread->state);
            free(name);

            while(last->prev != NULL) {
                for(; last->mb != NULL; last = last->prev) {
                    MethodBlock *mb = last->mb;
                    ClassBlock *cb = CLASS_CB(mb->class);
                    char *dot_name = slash2dots(cb->name);

                    printf("\tat %s.%s(", dot_name, mb->name);
                    free(dot_name);

                    if(mb->access_flags & ACC_NATIVE)
                        printf("Native method");
                    else
                        if(cb->source_file_name == NULL)
                            printf("Unknown source");
                        else {
                            int line = mapPC2LineNo(mb, last->last_pc);
                            printf("%s", cb->source_file_name);
                            if(line != -1)
                                printf(":%d", line);
                        }
                    printf(")\n");
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

    /* The signal handler thread must be a valid Java-level
       thread, as it needs to run the shutdown hooks in
       the event of user-termination of the VM */
    createVMThread("Signal Handler", dumpThreadsLoop);
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

extern char VM_initing;

void exitVM(int status) {
    main_exited = TRUE;

    /* Execute System.exit() to run any registered shutdown hooks.
       In the unlikely event that System.exit() can't be found, or
       it returns, fall through and exit. */

    if(!VM_initing) {
        Class *system = findSystemClass("java/lang/System");
        if(system) {
            MethodBlock *exit = findMethod(system, "exit", "(I)V");
            if(exit)
                executeStaticMethod(system, exit, status);
        }
    }

    exit(status);
}

void mainThreadWaitToExitVM() {
    Thread *self = threadSelf();
    TRACE(("Waiting for %d non-daemon threads to exit\n", non_daemon_thrds));

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

void initialiseMainThread(int stack_size) {
    Object *vmthread;
    Class *thrdGrp_class;
    MethodBlock *run, *remove_thread;
    FieldBlock *vmData, *vmThread, *thread, *daemon, *name, *group, *priority, *root;

    dflt_stack_size = stack_size;

    pthread_key_create(&threadKey, NULL);

    pthread_mutex_init(&lock, NULL);
    pthread_cond_init(&cv, NULL);

    pthread_mutex_init(&exit_lock, NULL);
    pthread_cond_init(&exit_cv, NULL);

    monitorInit(&sleep_mon);

    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);

    main_thread.stack_base = &thrdGrp_class;

    main_thread.tid = pthread_self();
    main_thread.id = genThreadID();
    main_thread.state = RUNNING;
    main_thread.ee = &main_ee;

    initialiseJavaStack(&main_ee);
    setThreadSelf(&main_thread);

    /* As we're initialising, VM will abort if Thread can't be found */
    thread_class = findSystemClass0("java/lang/Thread");

    vmThread = findField(thread_class, "vmThread", "Ljava/lang/VMThread;");
    daemon = findField(thread_class, "daemon", "Z");
    name = findField(thread_class, "name", "Ljava/lang/String;");
    group = findField(thread_class, "group", "Ljava/lang/ThreadGroup;");
    priority = findField(thread_class, "priority", "I");

    run = findMethod(thread_class, "run", "()V");

    vmthread_class = findSystemClass0("java/lang/VMThread");
    thread = findField(vmthread_class, "thread", "Ljava/lang/Thread;");
    vmData = findField(vmthread_class, "vmData", "I");

    /* findField and findMethod do not throw an exception... */
    if((vmData == NULL) || (run == NULL) || (daemon == NULL) || (name == NULL) ||
           (group == NULL) || (priority == NULL) || (vmThread == NULL) || (thread == NULL))
        goto error;

    vmthread_offset = vmThread->offset;
    thread_offset = thread->offset;
    vmData_offset = vmData->offset;
    daemon_offset = daemon->offset;
    group_offset = group->offset;
    priority_offset = priority->offset;
    name_offset = name->offset;
    run_mtbl_idx = run->method_table_index;

    vmthread = allocObject(vmthread_class);
    main_ee.thread = allocObject(thread_class);

    INST_DATA(vmthread)[vmData_offset] = (uintptr_t)&main_thread;
    INST_DATA(vmthread)[thread_offset] = (uintptr_t)main_ee.thread;

    INST_DATA(main_ee.thread)[daemon_offset] = FALSE;
    INST_DATA(main_ee.thread)[name_offset] = (uintptr_t)Cstr2String("main");
    INST_DATA(main_ee.thread)[priority_offset] = NORM_PRIORITY;
    INST_DATA(main_ee.thread)[vmthread_offset] = (uintptr_t)vmthread;

    thrdGrp_class = findSystemClass("java/lang/ThreadGroup");
    if(exceptionOccured()) {
        printException();
        exitVM(1);
    }

    root = findField(thrdGrp_class, "root", "Ljava/lang/ThreadGroup;");
    remove_thread = findMethod(thrdGrp_class, "removeThread", "(Ljava/lang/Thread;)V");

    /* findField and findMethod do not throw an exception... */
    if((root == NULL) || (remove_thread == NULL))
        goto error;

    rmveThrd_mtbl_idx = remove_thread->method_table_index;

    INST_DATA(main_ee.thread)[group_offset] = root->static_value;

    initialiseSignals();

    return;

error:
    fprintf(stderr, "Error initialising VM (initialiseMainThread)\n");
    exitVM(1);
}
