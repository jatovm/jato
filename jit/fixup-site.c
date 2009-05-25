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

#include <jit/compiler.h>
#include <arch/instruction.h>
#include <memory.h>
#include <malloc.h>

struct fixup_site *alloc_fixup_site(void)
{
	struct fixup_site *site;

	site = malloc(sizeof(*site));
	memset(site, 0, sizeof(*site));

	return site;
}

void free_fixup_site(struct fixup_site *site)
{
	free(site);
}

void trampoline_add_fixup_site(struct jit_trampoline *trampoline,
			       struct fixup_site *site)
{
	pthread_mutex_lock(&trampoline->mutex);
	list_add_tail(&site->fixup_list_node, &trampoline->fixup_site_list);
	pthread_mutex_unlock(&trampoline->mutex);
}

unsigned char *fixup_site_addr(struct fixup_site *site)
{
	return buffer_ptr(site->cu->objcode) + site->relcall_insn->mach_offset;
}
