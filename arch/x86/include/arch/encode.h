#ifndef X86_ENCODE_H
#define X86_ENCODE_H

#include "arch/registers.h"	/* for enum machine_reg */

#include <stdint.h>

uint8_t x86_encode_reg(enum machine_reg reg);

#endif /* X86_ENCODE_H */
