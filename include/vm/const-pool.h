#ifndef __VM_CONST_POOL_H
#define __VM_CONST_POOL_H

static inline void *cp_info_ptr(struct constant_pool *cp, unsigned short idx)
{
	return (void *) CP_INFO(cp, idx);
}

static inline unsigned short cp_index(unsigned char *code)
{
	return be16_to_cpu(*(u2 *) code);
}

#endif
