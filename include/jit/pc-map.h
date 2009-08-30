#ifndef JATO_JIT_PCMAP_H
#define JATO_JIT_PCMAP_H

#include <stdbool.h>

struct pc_map_entry {
	int nr_values;
	unsigned long *values;
};

struct pc_map {
	int size;
	struct pc_map_entry *map;
};

int pc_map_init_empty(struct pc_map *map, int size);
int pc_map_init_identity(struct pc_map *map, int size);
void pc_map_deinit(struct pc_map *map);
int pc_map_join(struct pc_map *map1, struct pc_map *map2);
int pc_map_get_unique(struct pc_map *map, unsigned long *pc);
int pc_map_add(struct pc_map *map, unsigned long from, unsigned long to);
bool pc_map_has_value_for(struct pc_map *map, unsigned long pc);
int pc_map_get_min_greater_than(struct pc_map *map, unsigned long key,
				unsigned long value, unsigned long *result);
int pc_map_get_max_lesser_than(struct pc_map *map, unsigned long key,
			       unsigned long value, unsigned long *result);
void pc_map_print(struct pc_map *map);

#define pc_map_for_each_value(pc_map, key, value_ptr)			\
	for ((value_ptr) = (pc_map)->map[key].values;			\
	     (value_ptr) < (pc_map)->map[key].values +			\
			 (pc_map)->map[key].nr_values;			\
	     value_ptr++)

#define pc_map_for_each_value_reverse(pc_map, key, value_ptr)		\
	for ((value_ptr) = (pc_map)->map[key].values +			\
				   (pc_map)->map[key].nr_values - 1;	\
	     (value_ptr) >= (pc_map)->map[key].values;			\
	     value_ptr--)

#endif
