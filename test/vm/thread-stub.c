#include "vm/thread.h"

__thread struct vm_exec_env *current_exec_env;

char *vm_thread_get_name(struct vm_thread *thread)
{
	return NULL;
}
