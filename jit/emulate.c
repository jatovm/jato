/*
 * Copyright (c) 2008, 2011  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "jit/emulate.h"

#include "jit/exception.h"
#include "vm/preload.h"
#include "vm/limits.h"
#include "vm/class.h"

#include <math.h>

int emulate_lcmp(long long value1, long long value2)
{
	if (value1 < value2)
		return -1;
	if (value2 < value1)
		return 1;
	return 0;
}

static int __emulate_fcmpx(float value1, float value2)
{
	float tmp;
	tmp	= value1 - value2;

	if (tmp < 0)
		return -1;
	else if (tmp > 0)
		return 1;

	return 0;
}

static int __emulate_dcmpx(double value1, double value2)
{
	double tmp;
	tmp	= value1 - value2;

	if (tmp < 0)
		return -1;
	else if (tmp > 0)
		return 1;

	return 0;
}

int emulate_fcmpl(float value1, float value2)
{
	if (isnanf(value1) || isnanf(value2))
		return -1;

	return __emulate_fcmpx(value1, value2);
}

int emulate_fcmpg(float value1, float value2)
{
	if (isnanf(value1) || isnanf(value2))
		return 1;

	return __emulate_fcmpx(value1, value2);
}

int emulate_dcmpl(double value1, double value2)
{
	if (isnan(value1) || isnan(value2))
		return -1;

	return __emulate_dcmpx(value1, value2);
}

int emulate_dcmpg(double value1, double value2)
{
	if (isnan(value1) || isnan(value2))
		return 1;

	return __emulate_dcmpx(value1, value2);
}

int32_t emulate_idiv(int32_t value1, int32_t value2)
{
	if (value2 == 0) {
		signal_new_exception(vm_java_lang_ArithmeticException, "division by zero");
		return 0;
	}

	return value1 / value2;
}

int32_t emulate_irem(int32_t value1, int32_t value2)
{
	if (value2 == 0) {
		signal_new_exception(vm_java_lang_ArithmeticException, "division by zero");
		return 0;
	}

	return value1 % value2;
}

long long emulate_ldiv(long long value1, long long value2)
{
	if (value2 == 0) {
		signal_new_exception(vm_java_lang_ArithmeticException,
					"division by zero");
		return 0;
	}

	return value1 / value2;
}

long long emulate_lrem(long long value1, long long value2)
{
	if (value2 == 0) {
		signal_new_exception(vm_java_lang_ArithmeticException,
					"division by zero");
		return 0;
	}

	return value1 % value2;
}

int32_t emulate_ishl(int32_t value1, int32_t value2)
{
	return value1 << (value2 & 0x3f);
}

int32_t emulate_ishr(int32_t value1, int32_t value2)
{
	return value1 >> (value2 & 0x3f);
}

int32_t emulate_iushr(int32_t value1, int32_t value2)
{
	return (uint32_t) value1 >> (value2 & 0x3f);
}

int64_t emulate_lshl(int64_t value1, int32_t value2)
{
	return value1 << (value2 & 0x3f);
}

int64_t emulate_lshr(int64_t value1, int32_t value2)
{
	return value1 >> (value2 & 0x3f);
}

int64_t emulate_lushr(int64_t value1, int32_t value2)
{
	return (uint64_t) value1 >> (value2 & 0x3f);
}

int32_t emulate_f2i(float value)
{
	if (finitef(value)) {
		if (value >= J_INT_MAX)
			return J_INT_MAX;
		if (value <= J_INT_MIN)
			return J_INT_MIN;

		return value;
	}

	if (isnanf(value))
		return 0;

	if (copysignf(1.0, value) > 0)
		return J_INT_MAX;

	return J_INT_MIN;
}

int64_t emulate_f2l(float value)
{
	if (finitef(value)) {
		if (value >= J_LONG_MAX)
			return J_LONG_MAX;
		if (value <= J_LONG_MIN)
			return J_LONG_MIN;

		return value;
	}

	if (isnanf(value))
		return 0;

	if (copysignf(1.0, value) > 0)
		return J_LONG_MAX;

	return J_LONG_MIN;
}

int32_t emulate_d2i(double value)
{
	if (finite(value)) {
		if (value >= J_INT_MAX)
			return J_INT_MAX;
		if (value <= J_INT_MIN)
			return J_INT_MIN;

		return value;
	}

	if (isnan(value))
		return 0;

	if (copysign(1.0, value) > 0)
		return J_INT_MAX;

	return J_INT_MIN;
}

int64_t emulate_d2l(double value)
{
	if (finite(value)) {
		if (value >= J_LONG_MAX)
			return J_LONG_MAX;
		if (value <= J_LONG_MIN)
			return J_LONG_MIN;

		return value;
	}

	if (isnan(value))
		return 0;

	if (copysign(1.0, value) > 0)
		return J_LONG_MAX;

	return J_LONG_MIN;
}
