/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
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

#pragma once

#ifndef CAFEBABE__ERROR_H
#define CAFEBABE__ERROR_H

enum cafebabe_errno {
	/* System-call-generated errors */
	CAFEBABE_ERROR_ERRNO,

	/* Cafebabe-generated errors */
	CAFEBABE_ERROR_EXPECTED_EOF,
	CAFEBABE_ERROR_UNEXPECTED_EOF,
	CAFEBABE_ERROR_BAD_MAGIC_NUMBER,
	CAFEBABE_ERROR_BAD_CONSTANT_TAG,
};

const char *cafebabe_strerror(enum cafebabe_errno e);

#endif
