#ifndef __TEST_VARS_H
#define __TEST_VARS_H

#define DECLARE_STATIC_REG(_var, _reg)						\
	static struct live_interval interval##_var;				\
										\
	static struct var_info _var = {						\
		.interval = &interval##_var,					\
	};									\
										\
	static struct live_interval interval##_var = {				\
		.var_info = &_var,						\
		.reg = _reg,							\
		.use_positions = LIST_HEAD_INIT(interval##_var.use_positions),	\
	};									\

#define DECLARE_STATIC_VREG(_var, _vreg)					\
	static struct live_interval interval##_var;				\
										\
	static struct var_info _var = {						\
		.vreg = _vreg,							\
		.interval = &interval##_var,					\
	};									\
										\
	static struct live_interval interval##_var = {				\
		.var_info = &_var,						\
		.use_positions = LIST_HEAD_INIT(interval##_var.use_positions),	\
		.reg = REG_UNASSIGNED,						\
	};									\

#endif
