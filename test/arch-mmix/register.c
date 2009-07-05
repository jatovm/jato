#include "arch/registers.h"
#include "jit/vars.h"

enum machine_reg_type reg_type(enum machine_reg reg)
{
	return REG_TYPE_GPR;
}
