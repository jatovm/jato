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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "jam.h"

static int VM_initing = TRUE;
extern void initialisePlatform();

/* Setup default values for command line args */

void setDefaultInitArgs(InitArgs *args) {
    args->noasyncgc = FALSE;

    args->verbosegc    = FALSE;
    args->verbosedll   = FALSE;
    args->verboseclass = FALSE;

    args->compact_specified = FALSE;

    args->classpath = NULL;
    args->bootpath  = NULL;

    args->java_stack = DEFAULT_STACK;
    args->min_heap   = DEFAULT_MIN_HEAP;
    args->max_heap   = DEFAULT_MAX_HEAP;

    args->props_count = 0;

    args->vfprintf = vfprintf;
    args->abort    = abort;
    args->exit     = exit;
}

int VMInitialising() {
    return VM_initing;
}

void initVM(InitArgs *args) {
    /* Perform platform dependent initialisation */
    initialisePlatform();

    /* Initialise the VM modules -- ordering is important! */
    initialiseHooks(args);
    initialiseProperties(args);
    initialiseAlloc(args);
    initialiseClass(args);
    initialiseDll(args);
    initialiseUtf8();
    initialiseMonitor();
    initialiseInterpreter();
    initialiseMainThread(args);
    initialiseException();
    initialiseString();
    initialiseGC(args);
    initialiseJNI();
    initialiseNatives();

    /* No need to check for exception - if one occurs, signalException aborts VM */
    findSystemClass("java/lang/NoClassDefFoundError");
    findSystemClass("java/lang/ClassFormatError");
    findSystemClass("java/lang/NoSuchFieldError");
    findSystemClass("java/lang/NoSuchMethodError");

    VM_initing = FALSE;
}

unsigned long parseMemValue(char *str) {
    char *end;
    unsigned long n = strtol(str, &end, 0);

    switch(end[0]) {
        case '\0':
            break;

        case 'M':
        case 'm':
            n *= MB;
            break;

        case 'K':
        case 'k':
            n *= KB;
            break;

        default:
             n = 0;
    } 

    return n;
}
