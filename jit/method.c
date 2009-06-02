/*
 * Copyright (c) 2009 Tomasz Grabiec
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

#include <jit/compiler.h>

#include <stdbool.h>

/* Points to the first address past text segment */
extern char etext;

/*
 * Checks whether address belongs to jitted or JATO method.
 * This is used in deciding when to stop the unwind process upon
 * exception throwing.
 *
 * It utilises the fact, that jitted code is allocated on heap. So by
 * comparing return address with text segment end we can tell whether
 * the caller is on heap or in text.
 */
bool is_jit_method(unsigned long eip)
{
	return eip >= (unsigned long)&etext;
}
