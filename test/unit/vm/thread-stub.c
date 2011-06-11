#include "vm/thread.h"

#include <pthread.h>

pthread_key_t current_exec_env_key;

__thread struct vm_exec_env *current_exec_env;

char *vm_thread_get_name(struct vm_thread *thread)
{
	return NULL;
}
