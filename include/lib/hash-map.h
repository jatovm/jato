#ifndef _JATO_HASH_MAP
#define _JATO_HASH_MAP

#include "lib/list.h"

struct hash_map_entry {
	const void *key;
	void *value;
	struct list_head list_node;
};

typedef unsigned long hash_fn(const void *, unsigned long);
typedef int compare_fn(const void *, const void *);

struct hash_map {
	hash_fn *hash;
	compare_fn *compare;

	unsigned long size;
	struct list_head *table;
};

struct hash_map *alloc_hash_map(unsigned long size, hash_fn *hash, compare_fn *compare);
void free_hash_map(struct hash_map *map);
int hash_map_put(struct hash_map *map, const void *key, void *value);
int hash_map_get(struct hash_map *map, const void *key, void **value_p);
int hash_map_remove(struct hash_map *map, const void *key);
bool hash_map_contains(struct hash_map *map, const void *key);
bool hash_map_is_empty(struct hash_map *map);

#define hash_map_for_each_entry(this, hashmap)				\
	for (unsigned long __i = 0; __i < (hashmap)->size; __i++)	\
		list_for_each_entry(this, &(hashmap)->table[__i], list_node)

hash_fn string_hash;
hash_fn ptr_hash;
compare_fn string_compare;
compare_fn ptr_compare;

#endif
