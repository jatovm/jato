#include "vm/class.h"
#include <stdio.h>

int main(void)
{
	printf("#define VTABLE_OFFSET \t%#x\n", offsetof(struct vm_class, vtable) + offsetof(struct vtable, native_ptr));
	return 0;
}
