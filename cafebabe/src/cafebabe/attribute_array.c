/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
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
