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
