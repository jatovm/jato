#ifndef JATO_STRING_H
#define JATO_STRING_H

struct vm_object;

void init_literals_hash_map(void);
struct vm_object *vm_string_intern(struct vm_object *string);

#endif /* JATO_STRING_H */
