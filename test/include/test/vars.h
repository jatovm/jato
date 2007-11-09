#ifndef __TEST_VARS_H
#define __TEST_VARS_H

#define DECLARE_REG(_var, _reg)		\
struct var_info _var = {			\
	.reg = _reg,				\
	.interval = { .var_info = &_var },	\
}

#define DECLARE_VREG(_var, _vreg)		\
struct var_info _var = {			\
	.vreg = _vreg,				\
	.interval = { .var_info = &_var },	\
}


#endif
