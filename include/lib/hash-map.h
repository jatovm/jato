#ifndef _JATO_HASH_MAP
#define _JATO_HASH_MAP

#include "lib/list.h"

struct hash_map_entry {
	const void *key;
	void *value;
	struct list_head list_node;
};

typedef unsigned long hash_fn(const void *);
typedef bool equals_fn(const void *, const void *);

struct key_operations {
	hash_fn *hash;
	equals_fn *equals;
};

struct key_operations pointer_key;
struct key_operations string_key;

struct hash_map {
	struct key_operations key_ops;
	unsigned long size;
	struct list_head *table;
};

struct hash_map *alloc_hash_map(unsigned long size, struct key_operations *key_ops);
void free_hash_map(struct hash_map *map);
int hash_map_put(struct hash_map *map, const void *key, void *value);
int hash_map_get(struct hash_map *map, const void *key, void **value_p);
int hash_map_remove(struct hash_map *map, const void *key);
bool hash_map_contains(struct hash_map *map, const void *key);

#define hash_map_for_each_entry(this, hashmap)				\
	for (unsigned long __i = 0; __i < (hashmap)->size; __i++)	\
		list_for_each_entry(this, &(hashmap)->table[__i], list_node)

#endif
