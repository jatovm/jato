#include "runtime/java_lang_VMThread.h"

#include "vm/preload.h"
#include "vm/object.h"
#include "vm/thread.h"

jobject java_lang_VMThread_currentThread(void)
{
	return field_get_object(vm_get_exec_env()->thread->vmthread, vm_java_lang_VMThread_thread);
}

void java_lang_VMThread_start(jobject vmthread, jlong stacksize)
{
	vm_thread_start(vmthread);
}

void java_lang_VMThread_yield(void)
{
	vm_thread_yield();
}

jboolean java_lang_VMThread_interrupted(void)
{
	return vm_thread_interrupted(vm_thread_self());
}

jboolean java_lang_VMThread_isInterrupted(jobject vmthread)
{
	struct vm_thread *thread;

	thread = vm_thread_from_vmthread(vmthread);

	return vm_thread_is_interrupted(thread);
}

void java_lang_VMThread_setPriority(jobject vmthread, jint priority)
{
	/* TODO: implement */
}

void java_lang_VMThread_interrupt(jobject vmthread)
{
	struct vm_thread *thread;

	thread = (struct vm_thread *) field_get_object(vmthread, vm_java_lang_VMThread_vmdata);

	vm_thread_interrupt(thread);
}
