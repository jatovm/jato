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

#include <jit/compilation-unit.h>
#include <jit/cu-mapping.h>

#include <vm/alloc.h>
#include <vm/radix-tree.h>
#include <vm/buffer.h>
#include <vm/die.h>

#include <pthread.h>

/*
 * A radix tree is used to associate native method addresses with
 * compilation unit. Only method entry address is stored in the tree
 * structure, so that inserting and removing compilation unit mapping
 * is fast.
 */
struct radix_tree *cu_map;
pthread_mutex_t cu_map_mutex = PTHREAD_MUTEX_INITIALIZER;

#define BITS_PER_LEVEL 6

void init_cu_mapping(void)
{
	int key_bits = sizeof(unsigned long) * 8;
	int viable_key_bits = key_bits - get_buffer_align_bits();

	cu_map = alloc_radix_tree(BITS_PER_LEVEL, viable_key_bits);
	if (!cu_map)
		die("out of memory");
}

static unsigned long addr_key(unsigned long addr)
{
	return addr >> get_buffer_align_bits();
}

static unsigned long cu_key(struct compilation_unit *cu)
{
	return addr_key((unsigned long)buffer_ptr(cu->objcode));
}

int add_cu_mapping(struct compilation_unit *cu)
{
	int result;

	pthread_mutex_lock(&cu_map_mutex);
	result = radix_tree_insert(cu_map, cu_key(cu), cu);
	pthread_mutex_unlock(&cu_map_mutex);

	return result;
}

void remove_cu_mapping(struct compilation_unit *cu)
{
	pthread_mutex_lock(&cu_map_mutex);
	radix_tree_remove(cu_map, cu_key(cu));
	pthread_mutex_unlock(&cu_map_mutex);
}

struct compilation_unit *get_cu_from_native_addr(unsigned long addr)
{
	struct compilation_unit *cu;
	unsigned long method_addr;

	pthread_mutex_lock(&cu_map_mutex);
	cu = radix_tree_lookup_prev(cu_map, addr_key(addr));
	pthread_mutex_unlock(&cu_map_mutex);

	if (cu == NULL)
		return NULL;

	method_addr = (unsigned long)buffer_ptr(cu->objcode);
	if (method_addr + buffer_offset(cu->objcode) <= addr)
		return NULL;

	return cu;
}
