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
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "../../jam.h"

char *nativeLibPath() {
    return getenv("LD_LIBRARY_PATH");
}

/* GNU Classpath's libraries end in .dylib because it
   uses libtool, but JNI libraries normally end in
   .jnilib under Mac OS X.  We try both.
*/

void *nativeLibOpen(char *path) {
    void *handle;
    int len = strlen(path);
    char buff[len + sizeof(".jnilib") + 1];
     
    strcpy(buff, path);
    strcpy(buff + len, ".dylib");

    if((handle = dlopen(buff, RTLD_LAZY)) == NULL) {
        strcpy(buff + len, ".jnilib");
        handle = dlopen(buff, RTLD_LAZY);
    }

    return handle;
}

void *nativeLibSym(void *handle, char *symbol) {
    return dlsym(handle, symbol);
}

char *nativeLibMapName(char *name) {
   char *buff = sysMalloc(strlen(name) + sizeof("lib") + 1);

   sprintf(buff, "lib%s", name);
   return buff;
}
