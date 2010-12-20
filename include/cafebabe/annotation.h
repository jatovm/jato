/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
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

#ifndef CAFEBABE__ANNOTATION_H
#define CAFEBABE__ANNOTATION_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

#define ELEMENT_TYPE_BYTE			'B'
#define ELEMENT_TYPE_CHAR			'C'
#define ELEMENT_TYPE_DOUBLE			'D'
#define ELEMENT_TYPE_FLOAT			'F'
#define ELEMENT_TYPE_INTEGER			'I'
#define ELEMENT_TYPE_LONG			'J'
#define ELEMENT_TYPE_SHORT			'S'
#define ELEMENT_TYPE_BOOLEAN			'Z'
#define ELEMENT_TYPE_STRING			's'
#define ELEMENT_TYPE_ENUM_CONSTANT		'e'
#define ELEMENT_TYPE_CLASS			'c'
#define ELEMENT_TYPE_ANNOTATION_TYPE		'@'
#define ELEMENT_TYPE_ARRAY			'['

struct cafebabe_annotation;

struct cafebabe_element_value {
	uint8_t					tag;			/* See ELEMENT_TYPE_<type> above. */
	union {
		uint16_t			const_value_index;
		struct {
			uint16_t		type_name_index;
			uint16_t		const_name_index;
		} enum_const_value;
		uint16_t			class_info_index;
		struct cafebabe_annotation	*annotation_value;
		struct {
			uint16_t			num_values;
			struct cafebabe_element_value	*values;
		} array_value;
	} value;
};

struct cafebabe_element_value_pair {
	uint16_t				element_name_index;
	struct cafebabe_element_value		value;
};

struct cafebabe_annotation {
	uint16_t				type_index;
	uint16_t				num_element_value_pairs;
	struct cafebabe_element_value_pair	*element_value_pairs;
};

#endif
