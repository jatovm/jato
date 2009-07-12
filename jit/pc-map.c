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

#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>

#include "vm/stdlib.h"
#include "jit/pc-map.h"

extern void print_trace(void);

/**
 * Insert pair (@from, @to) into relation pointed by @map.
 */
int pc_map_add(struct pc_map *map, unsigned long from, unsigned long to)
{
	struct pc_map_entry *ent;
	unsigned long *new_table;
	int new_size;

	assert(from < (unsigned long)map->size);

	ent = &map->map[from];

	new_size = sizeof(unsigned long) * (ent->nr_values + 1);
	new_table = realloc(ent->values, new_size);
	if (!new_table)
		return -ENOMEM;

	new_table[ent->nr_values++] = to;
	ent->values = new_table;

	return 0;
}

/**
 * Initialize @map to an empty relation
 */
int pc_map_init_empty(struct pc_map *map, int size)
{
	map->size = size;
	map->map = zalloc(sizeof(struct pc_map_entry) * size);
	if (!map->map)
		return -ENOMEM;

	return 0;
}

void pc_map_deinit(struct pc_map *map)
{
	for (int i = 0; i < map->size; i++)
		if (map->map[i].values)
			free(map->map[i].values);

	free(map->map);
}

/**
 * Initialize @map to an identity relation on [0 ; size]
 */
int pc_map_init_identity(struct pc_map *map, int size)
{
	int err;

	err = pc_map_init_empty(map, size);
	if (err)
		return err;

	for (int i = 0; i < size; i++) {
		err = pc_map_add(map, i, i);
		if (err) {
			pc_map_deinit(map);
			return err;
		}
	}

	return 0;
}

/**
 * Constructs a new relation representing the join of @map1 and @map2
 * and saves it in @map1. The join operation can be described as
 * follows: if pair (a, b) belongs to @map1 and tuble (b, c) belongs
 * to @map2 then pair (a, c) is inserted into the new map.
 */
int pc_map_join(struct pc_map *map1, struct pc_map *map2)
{
	struct pc_map result;
	int err;

	pc_map_init_empty(&result, map1->size);

	for (int i = 0; i < map1->size; i++) {
		unsigned long *val;

		pc_map_for_each_value(map1, i, val) {
			unsigned long *c;

			pc_map_for_each_value(map2, *val, c) {
				err = pc_map_add(&result, i, *c);
				if (err) {
					pc_map_deinit(&result);
					return -ENOMEM;
				}
			}
		}
	}

	pc_map_deinit(map1);
	*map1 = result;

	return 0;
}

/**
 * If @map contains a pair (*@pc, x) then *@pc is assigned x. If
 * there is no such pair or there exists another pair (*@pc, y)
 * that belongs to @map then error is returned.
 */
int pc_map_get_unique(struct pc_map *map, unsigned long *pc)
{
	if (*pc >= (unsigned long)map->size)
		return -1;

	if (map->map[*pc].nr_values != 1)
		return -1;

	*pc = map->map[*pc].values[0];
	return 0;
}

bool pc_map_has_value_for(struct pc_map *map, unsigned long pc)
{
	if (pc >= (unsigned long) map->size)
		return false;

	return map->map[pc].nr_values > 0;
}

int pc_map_get_min_greater_than(struct pc_map *map, unsigned long key,
				unsigned long value, unsigned long *result)
{
	unsigned long *val;
	bool assigned;

	assigned = false;

	pc_map_for_each_value(map, key, val) {
		if (*val < value)
			continue;

		if (*val < *result || !assigned) {
			*result = *val;
			assigned = true;
		}
	}

	return assigned ? 0 : -1;
}

int pc_map_get_max_lesser_than(struct pc_map *map, unsigned long key,
			       unsigned long value, unsigned long *result)
{
	unsigned long *val;
	bool assigned;

	assigned = false;

	pc_map_for_each_value_reverse(map, key, val) {
		if (*val > value)
			continue;

		if (*val > *result || !assigned) {
			*result = *val;
			assigned = true;
		}
	}

	return assigned ? 0 : -1;
}

void pc_map_print(struct pc_map *map)
{
	if (map->size == 0) {
		printf("(empty)\n");
		return;
	}

	for (unsigned long i = 0; i < (unsigned long) map->size; i++) {
		unsigned long *val;

		if (!pc_map_has_value_for(map, i))
			continue;

		pc_map_for_each_value(map, i, val) {
			printf("(%ld, %ld) ", i, *val);
		}
		printf("\n");
	}
}
