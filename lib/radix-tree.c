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

#include "lib/radix-tree.h"
#include "vm/stdlib.h"

#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

/*
 * Below is a schematic ilustration how values are mapped to keys in
 * radix tree. In this example it is assumed that key is 22 bits long
 * and bits_per_level is equal to 6.
 *
 *
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
 *                                                      |
 *                                                      v
 *                                                    value
 *
 */

static unsigned long level_mask(struct radix_tree *tree)
{
	return (1UL << tree->bits_per_level) - 1;
}

static int slot_count(struct radix_tree *tree)
{
	return 1 << tree->bits_per_level;
}

static int level_count(struct radix_tree *tree)
{
	return tree->level_count;
}

static struct radix_tree_node *
alloc_radix_tree_node(struct radix_tree *tree, struct radix_tree_node *parent)
{
	struct radix_tree_node *node;
	size_t size;

	size = sizeof(struct radix_tree_node *) * slot_count(tree);

	node = zalloc(sizeof(struct radix_tree_node) + size);
	if (!node)
		return NULL;

	node->parent = parent;

	return node;
}

struct radix_tree *
alloc_radix_tree(unsigned int bits_per_level, unsigned int key_bits)
{
	struct radix_tree *tree;

	assert(bits_per_level < key_bits);
	assert(key_bits <= sizeof(unsigned long) * 8);

	tree = malloc(sizeof(*tree));
	if (!tree)
		return NULL;

	tree->bits_per_level = bits_per_level;

	tree->level_count = (key_bits + bits_per_level - 1) / bits_per_level;

	tree->root = alloc_radix_tree_node(tree, NULL);
	if (!tree->root) {
		free(tree);
		return NULL;
	}

	return tree;
}

static void
free_radix_tree_node(struct radix_tree *tree, struct radix_tree_node *node,
		     int level)
{
	if (level < level_count(tree) - 1) {
		int i;

		for (i = 0; i < slot_count(tree); i++) {
			if (node->slots[i] == NULL)
				continue;

			free_radix_tree_node(tree, node->slots[i], level + 1);
		}
	}
	free(node);
}

void free_radix_tree(struct radix_tree *tree)
{
	if (!tree)
		return;

	free_radix_tree_node(tree, tree->root, 0);
	free(tree);
}

static void *
radix_tree_last(struct radix_tree *tree, struct radix_tree_node *node,
		int level)
{
	while (level < level_count(tree)) {
		int i;

		for (i = slot_count(tree) - 1; i >= 0; i--)
			if (node->slots[i] != NULL)
				break;

		if (i < 0)
			return NULL;

		node = node->slots[i];
		level++;
	}

	return node;
}

static unsigned long get_index(struct radix_tree *tree, unsigned long key,
			       int level)
{
	int shift = ((level_count(tree) - level - 1) * tree->bits_per_level);

	return (key >> shift) & level_mask(tree);
}

static void *
radix_tree_previous(struct radix_tree *tree, struct radix_tree_node *node,
		    unsigned long key, int level)
{
	int index;

	while (node) {
		index = get_index(tree, key, level) - 1;
		for (; index >= 0; index--) {
			if (node->slots[index] == NULL)
				continue;

			return radix_tree_last(tree, node->slots[index],
					       level + 1);
		}

		/*
		 * Go back one level until we find level with more
		 * than one slot filed in. We don't have to search
		 * level if there are no other slots.
		 */
		do {
			node = node->parent;
			level--;
		} while (node != NULL && node->count == 1);
	}

	return NULL;
}

/**
 * radix_tree_insert - Insert key->value mapping into the tree.
 *                     Returns 0 on success.
 *
 * @tree: a radix tree to put into.
 * @key: the search key
 * @value: the value to associate key with
 */
int radix_tree_insert(struct radix_tree *tree, unsigned long key, void *value)
{
	struct radix_tree_node *node = tree->root;
	int i;

	assert(value != NULL);

	for (i = 0; i < level_count(tree) - 1; i++) {
		int index = get_index(tree, key, i);

		if (node->slots[index] == NULL) {
			node->slots[index] = alloc_radix_tree_node(tree, node);
			if (node->slots[index] == NULL)
				return -ENOMEM;

			node->count++;
		}

		node = node->slots[index];
	}

	node->count++;
	node->slots[get_index(tree, key, i)] = value;

	return 0;
}

static void
free_slot(struct radix_tree *tree, struct radix_tree_node *node,
	  unsigned long key, int level)
{
	node->slots[get_index(tree, key, level)] = NULL;
	node->count--;

	if (node->count == 0 && node->parent != NULL) {
		free_slot(tree, node->parent, key, level - 1);
		free(node);
	}
}

/**
 * radix_tree_remove - remove mapping from tree.
 * @tree: a radix tree to remove from.
 * @key: a key to remove.
 */
void radix_tree_remove(struct radix_tree *tree, unsigned long key)
{
	struct radix_tree_node *node = tree->root;
	int i;

	for (i = 0; i < level_count(tree) - 1; i++) {
		int index = get_index(tree, key, i);

		if (node->slots[index] == NULL)
			return; /* no mapping exists */

		node = node->slots[index];
	}

	free_slot(tree, node, key, i);
}

static void *__radix_tree_lookup(struct radix_tree *tree, unsigned long key,
				 bool try_previous)
{
	struct radix_tree_node *node = tree->root;
	int i;

	for (i = 0; i < level_count(tree); i++) {
		int index = get_index(tree, key, i);

		if (node->slots[index] == NULL) {
			if (try_previous)
				return radix_tree_previous(tree, node, key, i);
			else
				return NULL;
		}

		node = node->slots[index];
	}

	return node;
}


/**
 * radix_tree_lookup - get the value associated with @key. Returns NULL when no
 *               mapping exists.
 *
 * @tree: a radix tree to lookup in.
 * @key: a key which value should be abtained.
 */
void *radix_tree_lookup(struct radix_tree *tree, unsigned long key)
{
	return __radix_tree_lookup(tree, key, false);
}

/**
 * radix_tree_lookup_prev - get the value associated with @key or
 *                          the value associated with the preceding key.
 *                          Returns NULL when no preceding key exists.
 *
 * @tree: a radix tree to lookup in.
 * @key: a key which value should be returned.
 */
void *radix_tree_lookup_prev(struct radix_tree *tree, unsigned long key)
{
	return __radix_tree_lookup(tree, key, true);
}
