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

#include "lib/hash-map.h"

#include <string.h>
#include <malloc.h>
#include <errno.h>

struct hash_map *alloc_hash_map(unsigned long size, hash_fn *hash, compare_fn *compare)
{
	struct hash_map *map;

	map = malloc(sizeof(*map));
	if (!map)
		return NULL;

	map->table = malloc(sizeof(struct list_head) * size);
	if (!map->table) {
		free(map);
		return NULL;
	}

	for (unsigned int i = 0; i < size; i++)
		INIT_LIST_HEAD(&map->table[i]);

	map->hash    = hash;
	map->compare = compare;
	map->size    = size;

	return map;
}

void free_hash_map(struct hash_map *map)
{
	free(map->table);
	free(map);
}

static inline struct hash_map_entry *
hash_map_lookup_entry(struct hash_map *map, const void *key)
{
	struct hash_map_entry *ent;
	struct list_head *bucket;

	bucket = &map->table[map->hash(key, map->size)];

	list_for_each_entry(ent, bucket, list_node)
		if (map->compare(ent->key, key) == 0)
			return ent;

	return NULL;
}

int hash_map_put(struct hash_map *map, const void *key, void *value)
{
	struct hash_map_entry *ent;
	struct list_head *bucket;

	ent = hash_map_lookup_entry(map, key);
	if (ent) {
		ent->value = value;
		return 0;
	}

	ent = malloc(sizeof(struct hash_map_entry));
	if (!ent)
		return -ENOMEM;

	ent->key   = key;
	ent->value = value;

	bucket = &map->table[map->hash(key, map->size)];
	list_add(&ent->list_node, bucket);

	return 0;
}

int hash_map_get(struct hash_map *map, const void *key, void **value_p)
{
	struct hash_map_entry *ent;

	ent = hash_map_lookup_entry(map, key);
	if (!ent)
		return -1;

	*value_p = ent->value;
	return 0;
}

int hash_map_remove(struct hash_map *map, const void *key)
{
	struct hash_map_entry *ent;

	ent = hash_map_lookup_entry(map, key);
	if (!ent)
		return -1;

	list_del(&ent->list_node);
	return 0;
}

unsigned long string_hash(const void *key, unsigned long size)
{
	unsigned long hash;
	const char *str;

	hash = 0;
	str  = key;

	while (*str)
		hash += 31 * hash + *str++;

	return hash % size;
}

int string_compare(const void *key1, const void *key2)
{
	return strcmp(key1, key2);
}
