/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007
 * Robert Lougher <rob@lougher.org.uk>.
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
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stddef.h>
#include "jam.h"

/* The function getCallerFrame() is used in the code that does
   security related stack-walking.  It guards against invocation
   via reflection.  These frames must be skipped, else it will
   appear that the caller was loaded by the boot loader. */

Frame *getCallerFrame(Frame *last) {

loop:
    /* Skip the top frame, and check if the
       previous is a dummy frame */
    if((last = last->prev)->mb == NULL) {

        /* Skip the dummy frame, and check if
         * we're at the top of the stack */
        if((last = last->prev)->prev == NULL)
            return NULL;

        /* Check if we were invoked via reflection */
        if(last->mb->class == getReflectMethodClass()) {

            /* There will be two frames for invoke.  Skip
               the first, and jump back.  This also handles
               recursive invocation via reflection. */

            last = last->prev;
            goto loop;
        }
    }
    return last;
}

Class *getCallerCallerClass() {
    Frame *last = getExecEnv()->last_frame->prev;

    if((last->mb == NULL && (last = last->prev)->prev == NULL) ||
             (last = getCallerFrame(last)) == NULL)
        return NULL;

    return last->mb->class;
}

