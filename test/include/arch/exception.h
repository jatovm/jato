#ifndef __ARCH_EXCEPTION_H
#define __ARCH_EXCEPTION_H

struct object;
struct compilation_unit;

unsigned char *throw_exception(struct compilation_unit *cu,
			       struct object *exception);

#endif /* __ARCH_EXCEPTION_H */
