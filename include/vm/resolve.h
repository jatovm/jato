#ifndef __VM_RESOLVE_H
#define __VM_RESOLVE_H

struct constant_pool;
struct object;

struct object *resolve_string(struct constant_pool *cp, unsigned long cp_idx);

#endif /* __VM_RESOLVE_H */
