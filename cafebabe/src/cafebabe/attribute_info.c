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

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"

int
cafebabe_attribute_info_init(struct cafebabe_attribute_info *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->attribute_name_index))
		goto out;

	if (cafebabe_stream_read_uint32(s, &a->attribute_length))
		goto out;

	a->info = cafebabe_stream_malloc(s, a->attribute_length);
	if (!a->info)
		goto out;

	for (uint16_t i = 0; i < a->attribute_length; ++i) {
		if (cafebabe_stream_read_uint8(s, &a->info[i]))
			goto out_info;
	}

	return 0;

out_info:
	free(a->info);
out:
	return 1;
}

void
cafebabe_attribute_info_deinit(struct cafebabe_attribute_info *a)
{
	free(a->info);
}
