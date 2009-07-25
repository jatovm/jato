#ifndef __X86_ARGS_H
#define __X86_ARGS_H

#include "arch/registers.h"

#include "vm/method.h"

#ifdef CONFIG_X86_64

extern int args_map_init(struct vm_method *method);

#endif /* CONFIG_X86_64 */

#endif /* __X86_ARGS_H */
