/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "jit/compiler.h"
#include "arch/instruction.h"
#include "vm/stdlib.h"

#include <memory.h>
#include <stdlib.h>

struct fixup_site *
alloc_fixup_site(struct compilation_unit *cu, struct insn *call_insn)
{
	struct fixup_site *site;

	site = zalloc(sizeof(*site));
	if (!site)
		return NULL;

	site->cu = cu;
	site->relcall_insn = call_insn;

	list_add(&site->list_node, &cu->call_fixup_site_list);

	return site;
}

void free_fixup_site(struct fixup_site *site)
{
	free(site);
}

unsigned char *fixup_site_addr(struct fixup_site *site)
{
	return buffer_ptr(site->cu->objcode) + site->mach_offset;
}
