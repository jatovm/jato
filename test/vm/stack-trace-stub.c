#include "vm/stack-trace.h"

int vm_enter_jni(void *caller_frame, unsigned long call_site_addr,
		 struct vm_method *method)
{
	return 0;
}

void vm_leave_jni(void)
{
}
