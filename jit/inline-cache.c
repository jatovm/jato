#include "arch/instruction.h"
#include "jit/compilation-unit.h"

#include "jit/inline-cache.h"

#include "lib/list.h"

#include <stdbool.h>

bool opt_ic_enabled = true;

bool ic_enabled(void)
{
	return opt_ic_enabled;
}

int add_ic_call(struct compilation_unit *cu, struct insn *insn)
{
	struct ic_call *ic_call;

	ic_call = malloc(sizeof(struct ic_call));
	if (!ic_call)
		return -ENOMEM;

	ic_call->insn = insn;

	list_add_tail(&ic_call->list_node, &cu->ic_call_list);

	return 0;
}
