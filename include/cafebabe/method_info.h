/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__METHOD_INFO_H
#define CAFEBABE__METHOD_INFO_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"

struct cafebabe_attribute_info;
struct cafebabe_stream;

/*
 * Method access and property flags.
 */

#define CAFEBABE_METHOD_ACC_PUBLIC		0x0001
#define CAFEBABE_METHOD_ACC_PRIVATE		0x0002
#define CAFEBABE_METHOD_ACC_PROTECTED		0x0004
#define CAFEBABE_METHOD_ACC_STATIC		0x0008
#define CAFEBABE_METHOD_ACC_FINAL		0x0010
#define CAFEBABE_METHOD_ACC_SYNCHRONIZED	0x0020
#define CAFEBABE_METHOD_ACC_BRIDGE		0x0040
#define CAFEBABE_METHOD_ACC_VARARGS		0x0080
#define CAFEBABE_METHOD_ACC_NATIVE		0x0100
#define CAFEBABE_METHOD_ACC_ABSTRACT		0x0400
#define CAFEBABE_METHOD_ACC_STRICT		0x0800
#define CAFEBABE_METHOD_ACC_SYNTHETIC		0x1000

/**
 * A java class method.
 *
 * See also section 4.6 of The Java Virtual Machine Specification.
 */
struct cafebabe_method_info {
	uint16_t access_flags;
	uint16_t name_index;
	uint16_t descriptor_index;
	struct cafebabe_attribute_array attributes;
};

int cafebabe_method_info_init(struct cafebabe_method_info *m,
	struct cafebabe_stream *s);
void cafebabe_method_info_deinit(struct cafebabe_method_info *m);

#endif
