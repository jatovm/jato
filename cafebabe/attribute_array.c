/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"

int cafebabe_attribute_array_get(const struct cafebabe_attribute_array *array,
	const char *name, const struct cafebabe_class *c, unsigned int *r)
{
	for (uint16_t i = *r; i < array->count; ++i) {
		const struct cafebabe_constant_info_utf8 *attribute_name;
		if (cafebabe_class_constant_get_utf8(c,
			array->array[i].attribute_name_index,
			&attribute_name))
		{
			return 1;
		}

		if (cafebabe_constant_info_utf8_compare(attribute_name, name))
			continue;

		*r = i;
		return 0;
	}

	/* Not found */
	return 1;
}
