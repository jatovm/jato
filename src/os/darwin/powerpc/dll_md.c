/*
 * Copyright (C) 2003, 2004, 2005 Robert Lougher <rob@lougher.demon.co.uk>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include "../../../jam.h"

/*
 This function calculates the size of the params area needed
 in the caller frame for the JNI native method.  This is: 
 JNIEnv + class (if static) + method arguments + saved regs +
 linkage area.  It's negative because the stack grows downwards.
*/

int nativeExtraArg(MethodBlock *mb) {
    int params = (mb->args_count + 1 +
                 ((mb->access_flags & ACC_STATIC) ? 1 : 0) + 7 + 6) * -4;

#ifdef DEBUG_DLL
    printf("<nativExtraArg %s%s : %d>\n", mb->name, mb->type, params);
#endif

    return params;
}
