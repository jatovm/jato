#include "vm/method.h"
#include "jit/args.h"

#include <stdlib.h>

#ifdef CONFIG_ARGS_MAP
int args_map_init(struct vm_method *method)
{
	size_t size;

	/* If the method isn't static, we have a *this. */
	size = method->args_count + !vm_method_is_static(method);

	method->args_map = malloc(sizeof(struct vm_args_map) * size);
	if (!method->args_map)
		return -1;

	return 0;
}
#endif
