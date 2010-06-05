#ifndef JATO_VM_UTF8_H
#define JATO_VM_UTF8_H

#include <stdint.h>

struct vm_object;

int utf8_char_count(const uint8_t *bytes, unsigned int n, unsigned int *res);
struct vm_object *utf8_to_char_array(const uint8_t *bytes, unsigned int n);
char *dots_to_slash(const char *utf);
char *slash_to_dots(const char *utf);

#endif /* JATO_VM_UTF8_H */
