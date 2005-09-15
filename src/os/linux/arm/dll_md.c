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

int nativeExtraArg(MethodBlock *mb) {
    int len = strlen(mb->type);
    if(mb->type[len-2] == ')')
        switch(mb->type[len-1]) {
            case 'V':
                return 0;
#if defined(__VFP_FP__) || defined(__SOFTFP__)
            case 'D':
            case 'J':
                return 1;
        }

    return 2;
#else
            case 'D':
                return 1;
            case 'F':
                return 2;
            case 'J':
                return 3;
        }

    return 4;
#endif
}
