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

#ifndef CAFEBABE__FIELD_INFO_H
#define CAFEBABE__FIELD_INFO_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"

struct cafebabe_attribute_info;
struct cafebabe_stream;

#define CAFEBABE_FIELD_ACC_PUBLIC		0x0001
#define CAFEBABE_FIELD_ACC_PRIVATE		0x0002
#define CAFEBABE_FIELD_ACC_PROTECTED		0x0004
#define CAFEBABE_FIELD_ACC_STATIC		0x0008
#define CAFEBABE_FIELD_ACC_FINAL		0x0010
#define CAFEBABE_FIELD_ACC_VOLATILE		0x0040
#define CAFEBABE_FIELD_ACC_TRANSIENT		0x0080

/**
 * A java class field.
 *
 * See also section 4.5 of The Java Virtual Machine Specification.
 */
struct cafebabe_field_info {
	uint16_t access_flags;
	uint16_t name_index;
	uint16_t descriptor_index;
	struct cafebabe_attribute_array attributes;
};

int cafebabe_field_info_init(struct cafebabe_field_info *fi,
	struct cafebabe_stream *s);
void cafebabe_field_info_deinit(struct cafebabe_field_info *fi);

#endif
