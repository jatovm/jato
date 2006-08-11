#ifndef __VM_NATIVES_H
#define __VM_NATIVES_H

int vm_register_native(const char *, const char *, void *);
void vm_unregister_natives(void);
void *vm_lookup_native(const char *, const char *);

#endif
