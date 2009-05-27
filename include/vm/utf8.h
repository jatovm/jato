#ifndef __VM_UTF8_H
#define __VM_UTF8_H

struct vm_object;

int utf8_char_count(const uint8_t *bytes, unsigned int n, unsigned int *res);
struct vm_object *utf8_to_char_array(const uint8_t *bytes, unsigned int n);

#endif
