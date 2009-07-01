#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <vm/object.h>
#include <vm/types.h>

int utf8_char_count(const uint8_t *bytes, unsigned int n, unsigned int *res)
{
	unsigned int result = 0;

	for (unsigned int i = 0; i < n; ++i) {
		++result;

		/* 0xxxxxxx: 1 byte */
		if (!(bytes[i] & 0x80)) {
			continue;
		}

		/* 110xxxxx: 2 bytes */
		if ((bytes[i] & 0xe0) == 0xc0) {
			if (i + 1 >= n)
				return 1;
			if ((bytes[++i] & 0xc0) != 0x80)
				return 1;
			continue;
		}

		/* 1110xxxx: 3 bytes */
		if ((bytes[i] & 0xf0) == 0xe0) {
			if (i + 2 >= n)
				return 1;
			if ((bytes[++i] & 0xc0) != 0x80)
				return 1;
			if ((bytes[++i] & 0xc0) != 0x80)
				return 1;
			continue;
		}

		/* Anything else is an error */
		return 1;
	}

	*res = result;
	return 0;
}

struct vm_object *utf8_to_char_array(const uint8_t *bytes, unsigned int n)
{
	unsigned int utf16_count;
	if (utf8_char_count(bytes, n, &utf16_count))
		return NULL;

	struct vm_object *array
		= vm_object_alloc_native_array(T_CHAR, utf16_count);
	if (!array) {
		NOT_IMPLEMENTED;
		return array;
	}

	uint16_t *utf16_chars = (uint16_t *) &array->fields;
	for (unsigned int i = 0, j = 0; i < n; ++i) {
		if (!(bytes[i] & 0x80)) {
			utf16_chars[j++] = bytes[i];
			continue;
		}

		if ((bytes[i] & 0xe0) == 0xc0) {
			uint16_t ch = (uint16_t) (bytes[i++] & 0x1f) << 6;
			ch += bytes[i++] & 0x3f;
			utf16_chars[j++] = ch;
			continue;
		}

		if ((bytes[i] & 0xf0) == 0xe0) {
			uint16_t ch = (uint16_t) (bytes[i++] & 0xf) << 12;
			ch += (uint16_t) (bytes[i++] & 0x3f) << 6;
			ch += bytes[i++] & 0x3f;
			utf16_chars[j++] = ch;
			continue;
		}
	}

	return array;
}

char *slash2dots(char *utf8)
{
	char *result = strdup(utf8);

	for (unsigned int i = 0, n = strlen(utf8); i < n; ++i) {
		if (result[i] == '/')
			result[i] = '.';
	}

	return result;
}
