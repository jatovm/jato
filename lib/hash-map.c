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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "vm/die.h"

#define DEFAULT_CAPACITY 16

struct hash_map *alloc_hash_map(struct key_operations *key_ops)
{
	return alloc_hash_map_with_size(DEFAULT_CAPACITY, key_ops);
}

struct hash_map *alloc_hash_map_with_size(unsigned long initial_size, struct key_operations *key_ops)
{
	struct hash_map *map;

	map = malloc(sizeof(*map));
	if (!map)
		return NULL;

	map->table = malloc(sizeof(struct list_head) * initial_size);
	if (!map->table) {
		free(map);
		return NULL;
	}

	for (unsigned int i = 0; i < initial_size; i++)
		INIT_LIST_HEAD(&map->table[i]);

	map->key_ops		= *key_ops;
	map->table_size		= initial_size;
	map->size		= 0;
	map->fill_factor	= 0.75;
	map->resize_at		= initial_size * map->fill_factor;

	return map;
}

void free_hash_map(struct hash_map *map)
{
	for (unsigned int i = 0; i < map->table_size; i++) {
		struct list_head *bucket = &map->table[i];
		struct hash_map_entry *ent, *next;

		list_for_each_entry_safe(ent, next, bucket, list_node) {
			if (map->key_ops.final_process)
				map->key_ops.final_process(ent->value);
			free(ent);
		}
	}

	free(map->table);
	free(map);
}

static inline unsigned long get_hash(const struct hash_map *map, const void *key)
{
	return map->key_ops.hash(key);
}

static inline unsigned long get_index(const struct hash_map *map, unsigned long hash)
{
	return hash % map->table_size;
}

static inline struct hash_map_entry *
hash_map_lookup_entry(struct hash_map *map, const void *key)
{
	struct hash_map_entry *ent;
	struct list_head *bucket;
	unsigned long hash;

	hash = get_hash(map, key);

	bucket = &map->table[get_index(map, hash)];

	list_for_each_entry(ent, bucket, list_node) {
		if (hash == ent->hash && map->key_ops.equals(ent->key, key))
			return ent;
	}

	return NULL;
}

static void try_to_expand(struct hash_map *map)
{
	struct list_head *old_table = map->table;
	unsigned int old_table_size = map->table_size;
	unsigned int new_table_size;

	new_table_size = old_table_size * 2 + 1;

	struct list_head *new_table = malloc(sizeof(struct list_head) * new_table_size);
	if (!new_table) {
		free(new_table);
		warn("Failed to expand hash map");
		return;
	}

	for (unsigned int i = 0; i < new_table_size; i++)
		INIT_LIST_HEAD(&new_table[i]);

	map->table_size	= new_table_size;
	map->resize_at	= map->fill_factor * new_table_size;
	map->table	= new_table;

	for (unsigned int i = 0; i < old_table_size; i++) {
		struct hash_map_entry *ent, *next;

		list_for_each_entry_safe(ent, next, &old_table[i], list_node) {
			ent->hash = get_hash(map, ent->key);

			list_del(&ent->list_node);
			list_add(&ent->list_node, &new_table[get_index(map, ent->hash)]);
		}
	}

	free(old_table);
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

	if (map->size + 1 >= map->resize_at)
		try_to_expand(map);

	ent = malloc(sizeof(struct hash_map_entry));
	if (!ent)
		return -ENOMEM;

	ent->key   = key;
	ent->value = value;
	ent->hash  = get_hash(map, key);
	INIT_LIST_HEAD(&ent->list_node);

	bucket = &map->table[get_index(map, ent->hash)];
	list_add(&ent->list_node, bucket);
	map->size++;
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
	if (map->key_ops.final_process)
		map->key_ops.final_process(ent->value);
	free(ent);
	map->size--;
	return 0;
}

bool hash_map_contains(struct hash_map *map, const void *key)
{
	return hash_map_lookup_entry(map, key) != NULL;
}

int hash_map_size(struct hash_map *map)
{
	return map->size;
}

bool hash_map_is_empty(struct hash_map *map)
{
	return map->size == 0;
}


static unsigned long string_hash(const void *key)
{
	unsigned long hash;
	const char *str;

	hash = 0;
	str  = key;

	while (*str)
		hash = 31 * hash + *str++;

	return hash;
}

unsigned long ptr_hash(const void *key)
{
	return (unsigned long) key;
}

bool ptr_equals(const void *p1, const void *p2)
{
	return p1 == p2;
}

static bool string_equals(const void *key1, const void *key2)
{
	return strcmp(key1, key2) == 0;
}

struct key_operations pointer_key = {
	.hash	= ptr_hash,
	.equals	= ptr_equals
};

struct key_operations string_key = {
	.hash	= string_hash,
	.equals	= string_equals
};
