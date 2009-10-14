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

#pragma once

#ifndef CAFEBABE__CODE_ATTRIBUTE_H
#define CAFEBABE__CODE_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

struct cafebabe_code_attribute_exception {
	uint16_t start_pc;
	uint16_t end_pc;
	uint16_t handler_pc;
	uint16_t catch_type;
};

/**
 * The Code attribute.
 *
 * See also section 4.7.3 of The Java Virtual Machine Specification.
 */
struct cafebabe_code_attribute {
	uint16_t max_stack;
	uint16_t max_locals;

	uint32_t code_length;
	uint8_t *code;

	uint16_t exception_table_length;
	struct cafebabe_code_attribute_exception *exception_table;

	struct cafebabe_attribute_array attributes;
};

int cafebabe_code_attribute_init(struct cafebabe_code_attribute *a,
	struct cafebabe_stream *s);
void cafebabe_code_attribute_deinit(struct cafebabe_code_attribute *a);

#endif
