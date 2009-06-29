#ifndef __VM_THREAD_H
#define __VM_THREAD_H

#include <stdio.h> /* for NOT_IMPLEMENTED */

struct vm_object;

struct vm_thread {
	struct vm_object *exception;
};

struct vm_thread *vm_current_thread(void)
{
	static struct vm_thread tmp;

	NOT_IMPLEMENTED;
	return &tmp;
}

#endif
