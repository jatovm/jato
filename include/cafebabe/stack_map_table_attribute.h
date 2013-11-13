/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2011 Theo Dzierzbicki <theo.dz@gmail.com>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__STACK_MAP_TABLE_ATTRIBUTE_H
#define CAFEBABE__STACK_MAP_TABLE_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

enum cafebabe_verification_type_info_tag {
	CAFEBABE_VERIFICATION_TAG_TOP_VARIABLE_INFO = 1,
	CAFEBABE_VERIFICATION_TAG_INTEGER_VARIABLE_INFO = 2,
	CAFEBABE_VERIFICATION_TAG_FLOAT_VARIABLE_INFO = 3,
	CAFEBABE_VERIFICATION_TAG_LONG_VARIABLE_INFO = 4,
	CAFEBABE_VERIFICATION_TAG_DOUBLE_VARIABLE_INFO = 5,
	CAFEBABE_VERIFICATION_TAG_NULL_VARIABLE_INFO = 6,
	CAFEBABE_VERIFICATION_TAG_UNINITIALIZEDTHIS_VARIABLE_INFO = 6,
	CAFEBABE_VERIFICATION_TAG_OBJECT_VARIABLE_INFO = 7,
	CAFEBABE_VERIFICATION_TAG_UNINITIALIZED_VARIABLE_INFO = 8,
};

struct cafebabe_verification_type_info_object_variable {
	uint16_t cpool_index;
};

struct cafebabe_verification_type_info_uninitialized_variable {
	uint16_t offset;
};

struct cafebabe_verification_type_info {
	enum cafebabe_verification_type_info_tag tag;
	union {
		struct cafebabe_verification_type_info_object_variable object;
		struct cafebabe_verification_type_info_uninitialized_variable uninitialized;
	};
};

/* No need for a same_frame structure since it would be empty (all the
 * information it contains is the offset_delta). Moreover, since we store the
 * offset_delta outside the frame structure there is no need for extended
 * frame structures structures either.
 */

enum cafebabe_stack_map_frame_tag {
	CAFEBABE_STACK_MAP_TAG_SAME_FRAME,
	CAFEBABE_STACK_MAP_TAG_SAME_LOCAlS_1_STACK_ITEM_FRAME,
	CAFEBABE_STACK_MAP_TAG_CHOP_FRAME,
	CAFEBABE_STACK_MAP_TAG_APPEND_FRAME,
	CAFEBABE_STACK_MAP_TAG_FULL_FRAME,
};

struct cafebabe_stack_map_same_locals_1_stack_item_frame  {
	struct cafebabe_verification_type_info stack[1];
};

struct cafebabe_stack_map_chop_frame {
	uint8_t chopped;
};

struct cafebabe_stack_map_append_frame {
	uint8_t nr_locals;
	struct cafebabe_verification_type_info *locals;
};

struct cafebabe_stack_map_full_frame {
	uint16_t nr_locals;
	uint16_t nr_stack_items;
	struct cafebabe_verification_type_info *locals;
	struct cafebabe_verification_type_info *stack;
};

struct cafebabe_stack_map_frame_entry {
	enum cafebabe_stack_map_frame_tag tag;
	uint16_t offset_delta;
	union {
		struct cafebabe_stack_map_same_locals_1_stack_item_frame same_locals_1_stack_item_frame;
		struct cafebabe_stack_map_chop_frame chop_frame;
		struct cafebabe_stack_map_append_frame append_frame;
		struct cafebabe_stack_map_full_frame full_frame;
	};
};

/*
 * The StackMapTable attribute.
 *
 * See also section 4.8.4 of The Java Virtual Machine Specification.
 */
struct cafebabe_stack_map_table_attribute {
	uint16_t stack_map_frame_length;
	struct cafebabe_stack_map_frame_entry *stack_map_frame;
};

int cafebabe_stream_read_verification_type_info(struct cafebabe_stream *s, struct cafebabe_verification_type_info *info);

int cafebabe_stack_map_table_attribute_init(struct cafebabe_stack_map_table_attribute *a, struct cafebabe_stream *s);
void cafebabe_stack_map_table_attribute_deinit(struct cafebabe_stack_map_table_attribute *a);
int cafebabe_read_stack_map_table_attribute(
	const struct cafebabe_class *class,
	const struct cafebabe_attribute_array *attributes,
	struct cafebabe_stack_map_table_attribute *stack_map_table_attrib);

#endif
