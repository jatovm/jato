#include "jit/cu-mapping.h"
#include "jit/exception.h"

#include "lib/guard-page.h"

#include "vm/class.h"
#include "vm/object.h"
#include "vm/signal.h"

#include "arch/instruction.h"

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

void *static_guard_page;

void static_fixup_init(void)
{
	static_guard_page = alloc_guard_page();
	if (!static_guard_page)
		abort();
}

static int
add_static_fixup_site(enum static_fixup_type type, struct insn *insn,
		      struct vm_field *vmf, struct compilation_unit *cu)
{
	struct vm_class *vmc;
	struct static_fixup_site *site;

	vmc = vmf->class;

	site = malloc(sizeof *site);
	if (!site)
		return -ENOMEM;

	site->type = type;
	site->insn = insn;
	site->vmf = vmf;
	site->cu = cu;

	pthread_mutex_lock(&vmc->mutex);
	list_add_tail(&site->vmc_node, &vmc->static_fixup_site_list);
	pthread_mutex_unlock(&vmc->mutex);

	list_add_tail(&site->cu_node, &cu->static_fixup_site_list);
	return 0;
}

int add_getstatic_fixup_site(struct insn *insn,
	struct vm_field *vmf, struct compilation_unit *cu)
{
	return add_static_fixup_site(STATIC_FIXUP_GETSTATIC, insn, vmf, cu);
}

int add_putstatic_fixup_site(struct insn *insn,
	struct vm_field *vmf, struct compilation_unit *cu)
{
	return add_static_fixup_site(STATIC_FIXUP_PUTSTATIC, insn, vmf, cu);
}

unsigned long static_field_signal_bh(unsigned long ret)
{
	if (fixup_static_at(ret))
		return throw_from_signal_bh(ret);

	return ret;
}

