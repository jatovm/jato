/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"

int cafebabe_attribute_array_get(const struct cafebabe_attribute_array *array,
	const char *name, const struct cafebabe_class *c, unsigned int *r)
{
	for (uint16_t i = 0; i < array->count; ++i) {
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
