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

/* Must be included first to get configure options */
#include "jam.h"

#ifdef USE_FFI
#include <ffi.h>
#include <stdio.h>
#include "sig.h"

ffi_type *sig2ffi(char sig) {
    ffi_type *type;

    switch(sig) {
        case 'V':
	    type = &ffi_type_void;
            break;
        case 'D':
	    type = &ffi_type_double;
            break;
        case 'J':
	    type = &ffi_type_sint64;
            break;
        case 'F':
	    type = &ffi_type_float;
            break;
	default:
	    type = &ffi_type_pointer;
            break;
    }

    return type;
}

int nativeExtraArg(MethodBlock *mb) {
    char *sig = mb->type;
    int count = 2;

    SCAN_SIG(sig, count++, count++);

    return count;
}

#define DOUBLE                    \
    *types_pntr++ = sig2ffi(*sig);\
    *values_pntr++ = opntr;       \
    opntr += 2

#define SINGLE                    \
    *types_pntr++ = sig2ffi(*sig);\
    *values_pntr++ = opntr++

uintptr_t *callJNIMethod(void *env, Class *class, char *sig, int num_args, uintptr_t *ostack,
                         unsigned char *func) {
    ffi_cif cif;
    void *values[num_args];
    ffi_type *types[num_args];
    uintptr_t *opntr = ostack;
    void **values_pntr = &values[2];
    ffi_type **types_pntr = &types[2];

    types[0] = &ffi_type_pointer;
    values[0] = &env;

    types[1] = &ffi_type_pointer;
    values[1] = class ? &class : (void*)opntr++;

    SCAN_SIG(sig, DOUBLE, SINGLE);

    if(ffi_prep_cif(&cif, FFI_DEFAULT_ABI, num_args, sig2ffi(*sig), types) == FFI_OK) {
        ffi_call(&cif, FFI_FN(func), ostack, values);

	if(*sig == 'J' || *sig == 'D')
	    ostack += 2;
	else if(*sig != 'V')
	    ostack++;
    }

    return ostack;
}
#endif
