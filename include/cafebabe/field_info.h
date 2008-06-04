#pragma once

#ifndef CAFEBABE__FIELD_INFO_H
#define CAFEBABE__FIELD_INFO_H

#include <stdint.h>

struct cafebabe_attribute_info;
struct cafebabe_stream;

struct cafebabe_field_info {
	uint16_t access_flags;
	uint16_t name_index;
	uint16_t descriptor_index;
	uint16_t attributes_count;
	struct cafebabe_attribute_info *attributes;
};

int cafebabe_field_info_init(struct cafebabe_field_info *fi,
	struct cafebabe_stream *s);
void cafebabe_field_info_deinit(struct cafebabe_field_info *fi);

#endif
