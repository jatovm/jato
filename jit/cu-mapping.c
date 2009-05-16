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

#include <vm/buffer.h>
#include <vm/stdlib.h>
#include <vm/die.h>

#include <unistd.h>
#include <malloc.h>

/*
 * A radix tree is used to associate native method addresses with
 * compilation unit. Only method entry address is stored in the tree
 * structure, so that inserting and removing compilation unit mapping
 * is fast. When we want to find to which compilation unit given
 * address belongs we first check the location corresponding to that
 * address.  If method code size is less than buffer alignment size
 * (typically 4096 bytes) then compilation unit will be found
 * instantly. For larger methods we might end up on some level with no
 * mapping present for given index. When that happens we search for
 * the predecessor of that element. Predecessor search time is
 * proportionall with the distance of the address from method entry.
 *
 * Below is a schematic ilustration how pointer to compilation_unit is
 * stored in the structure based on it's method entry address. In this
 * example it is assumed that address is 32-bit long, buffer
 * alignment (page size) is 2^10 and KEY_BITS is equal to 6.
 *
 *
 *                                            /----------/-- buffer alignment
 *                                            |          |
 *  buffer_ptr --> xxxx xxxx xxxx xxxx xxxx xx00 0000 0000
 *                  \                         \
 *                   \                         \
 *                    \                         \
 *                     \                         \
 *                      \                         \
 *                       \                         \
 *  key -->   0000 0000 00xx xxxx xxxx xxxx xxxx xxxx
 *                      |     ||     | |     ||     |
 *                      \-----/\-----/ \-----/\-----/
 *                       |       |         |    |
 *                      /       /          |    |
 *                     /       /           |     \
 *                    /       /            |      \
 *                   /        |            |       \
 *                  /         |            |        \
 *                 |          |            |         \
 *       +----+-+-+-+-+..+-+  |            |          \
 * root: |NULL| | | | |  | |  |            |           \
 *       +----+-+-+-+-+..+-+  |            |            |
 *       level 0   |          |            |            |
 *                 |  +----+-+-+-+-+..+-+  |            |
 *                  > |    | | | | |  | |  |            |
 *                    +----+-+-+-+-+..+-+  |            |
 *                    level 1 |            |            |
 *                            |    +----+-+-+-+-+..+-+  |
 *                             ->  |    | | | | |  | |  |
 *                                 +----+-+-+-+-+..+-+  |
 *                                 level 2 |            |
 *                                         |    +----+-+-+-+-+..+-+
 *                                          ->  |    | | | | |  | |
 *                                              +----+-+-+-+-+..+-+
 *                                              level 3 |
 *                                                      v
 *                                             +------------------+
 *                                             | compilation_unit |
 *                                             +------------------+
 *
 */

#define KEY_BITS 6
#define SLOT_COUNT (1 << KEY_BITS)

const unsigned long key_mask = ((1ul << KEY_BITS) - 1);
int level_count;

struct radix_tree_node {
	struct radix_tree_node *parent;
	int count; /* number of nonempty slots */
	void *slots[SLOT_COUNT];
};

static int buffer_alignment_bits;
struct radix_tree_node *root;

static struct radix_tree_node *
alloc_radix_tree_node(struct radix_tree_node *parent)
{
	struct radix_tree_node *node;

	node = zalloc(sizeof(struct radix_tree_node));
	if (!node)
		return NULL;

	node->parent = parent;

	return node;
}

void init_cu_mapping(void)
{
	int page_size = getpagesize();
	int viable_key_bits;

	while (page_size > 0) {
		buffer_alignment_bits++;
		page_size >>= 1;
	}

	viable_key_bits = sizeof(unsigned long) * 8 - buffer_alignment_bits;
	level_count = (viable_key_bits + KEY_BITS - 1) / KEY_BITS;

	root = alloc_radix_tree_node(NULL);
	if (!root)
		die("out of memory");
}

static void *tree_last(struct radix_tree_node *node, int level)
{
	int i;

	while (level < level_count) {
		for (i = SLOT_COUNT - 1; i >= 0; i--)
			if (node->slots[i] != NULL) {
				node = node->slots[i];
				level++;
				break;
			}

		if (i < 0)
			return NULL;
	}

	return node;
}

static unsigned long get_index(unsigned long key, int level)
{
	int shift = ((level_count - level - 1) * KEY_BITS);

	return (key >> shift) & key_mask;
}

static void *tree_previous(struct radix_tree_node *node, unsigned long key,
			   int level)
{
	unsigned long index;

	/* we don't have to search this level if there are no
	   other slots */
	while (node != NULL && node->count == 1) {
		node = node->parent;
		level--;
	}

	if (node == NULL)
		return NULL;

	for (index = get_index(key, level); index >= 0; index--)
		if (node->slots[index] != NULL)
			return tree_last(node->slots[index], level + 1);

	return tree_previous(node->parent, key, level - 1);
}

void add_cu_mapping(struct compilation_unit *cu)
{
	struct radix_tree_node *node = root;
	unsigned long key;
	int i;

	key = (unsigned long)buffer_ptr(cu->objcode) >> buffer_alignment_bits;

	for (i = 0; i<level_count-1; i++) {
		int index = get_index(key, i);

		if (node->slots[index] == NULL)
			node->slots[index] = alloc_radix_tree_node(node);

		node = node->slots[index];
	}

	node->count++;
	node->slots[get_index(key, i)] = cu;
}

static void free_slot(struct radix_tree_node *node, int key, int level)
{
	node->slots[get_index(key, level)] = NULL;
	node->count--;

	if (node->count == 0 && node->parent != NULL) {
		free(node);
		free_slot(node->parent, key, level - 1);
	}
}

void remove_cu_mapping(struct compilation_unit *cu)
{
	int i;
	struct radix_tree_node *node = root;
	unsigned long key;

	key = (unsigned long)buffer_ptr(cu->objcode) >> buffer_alignment_bits;

	for (i = 0; i < level_count - 1; i++) {
		int index = get_index(key, i);

		if (node->slots[index] == NULL)
			return; /* no mapping exists */

		node = node->slots[index];
	}

	free_slot(node, key, i);
}

static struct compilation_unit *find_cu(unsigned long addr)
{
	int i;
	struct radix_tree_node *node = root;
	unsigned long key;

	key = addr >> buffer_alignment_bits;

	for (i = 0; i < level_count; i++) {
		int index = get_index(key, i);

		if (node->slots[index] == NULL)
			return tree_previous(node, key, i);

		node = node->slots[index];
	}

	return (struct compilation_unit *)node;
}

struct compilation_unit *get_cu_from_native_addr(unsigned long addr)
{
	struct compilation_unit *cu = find_cu(addr);
	unsigned long method_addr = (unsigned long)buffer_ptr(cu->objcode);

	if (method_addr + buffer_offset(cu->objcode) <= addr)
		return NULL;

	return cu;
}
