/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
