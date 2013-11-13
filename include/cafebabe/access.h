/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__ACCESS_H
#define CAFEBABE__ACCESS_H

/**
 * Access flags.
 *
 * See also tables 4.1, 4.4, 4.5, and 4.7 of The Java Virtual Machine
 * Specification.
 */
enum cafebabe_access_flags {
	CAFEBABE_ACCESS_PUBLIC		= 0x0001,
	CAFEBABE_ACCESS_PRIVATE		= 0x0002,
	CAFEBABE_ACCESS_PROTECTED	= 0x0004,
	CAFEBABE_ACCESS_STATIC		= 0x0008,
	CAFEBABE_ACCESS_FINAL		= 0x0010,
	CAFEBABE_ACCESS_SUPER		= 0x0020,
	CAFEBABE_ACCESS_SYNCHRONIZED	= 0x0020,
	CAFEBABE_ACCESS_VOLATILE	= 0x0040,
	CAFEBABE_ACCESS_TRANSIENT	= 0x0080,
	CAFEBABE_ACCESS_NATIVE		= 0x0100,
	CAFEBABE_ACCESS_INTERFACE	= 0x0200,
	CAFEBABE_ACCESS_ABSTRACT	= 0x0400,
	CAFEBABE_ACCESS_STRICT		= 0x0800,
};

#endif
