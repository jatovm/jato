/*
 * Copyright (c) 2008  Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <jit/exception.h>
#include <jit/emulate.h>
#include <vm/class.h>

int emulate_lcmp(long long value1, long long value2)
{
	long long tmp = value1 - value2;

	if (tmp < 0)
		return -1;
	if (tmp > 0)
		return 1;
	return 0;
}

long long emulate_ldiv(long long value1, long long value2)
{
	if (value2 == 0) {
		signal_new_exception(
			"java/lang/ArithmeticException", "division by zero");
		throw_from_native(2 * sizeof(long long));
		return 0;
	}

	return value1 / value2;
}

long long emulate_lrem(long long value1, long long value2)
{
	if (value2 == 0) {
		signal_new_exception(
			"java/lang/ArithmeticException", "division by zero");
		throw_from_native(2 * sizeof(long long));
		return 0;
	}

	return value1 % value2;
}
