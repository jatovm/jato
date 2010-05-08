/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "jit/compiler.h"
#include "arch/instruction.h"
#include "vm/stdlib.h"

#include <memory.h>
#include <malloc.h>

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
