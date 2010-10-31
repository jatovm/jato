#ifndef JATO__JAVA_LANG_VMSYSTEM_H
#define JATO__JAVA_LANG_VMSYSTEM_H

#include <stdint.h>

struct vm_object;

void native_vmsystem_arraycopy(struct vm_object *src, int src_start, struct vm_object *dest, int dest_start, int len);
int32_t native_vmsystem_identityhashcode(struct vm_object *obj);

#endif /* JATO__JAVA_LANG_VMSYSTEM_H */
