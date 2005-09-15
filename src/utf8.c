/*
 * Copyright (C) 2003, 2004 Robert Lougher <rob@lougher.demon.co.uk>.
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

#include <string.h>
#include <stdlib.h>
#include "jam.h"
#include "hash.h"

#define HASHTABSZE 1<<10
#define HASH(ptr) utf8Hash(ptr)
#define COMPARE(ptr1, ptr2, hash1, hash2) (ptr1 == ptr2) || \
                  ((hash1 == hash2) && utf8Comp(ptr1, ptr2))
#define PREPARE(ptr) ptr
#define SCAVENGE(ptr) FALSE
#define FOUND(ptr)

static HashTable hash_table;

#define GET_UTF8_CHAR(ptr, c)                         \
{                                                     \
    int x = *ptr++;                                   \
    if(x & 0x80) {                                    \
        int y = *ptr++;                               \
        if(x & 0x20) {                                \
            int z = *ptr++;                           \
            c = ((x&0xf)<<12)+((y&0x3f)<<6)+(z&0x3f); \
        } else                                        \
            c = ((x&0x1f)<<6)+(y&0x3f);               \
    } else                                            \
        c = x;                                        \
}

int utf8Len(unsigned char *utf8) {
    int count;

    for(count = 0; *utf8; count++) {
        int x = *utf8;
        utf8 += (x & 0x80) ? ((x & 0x20) ?  3 : 2) : 1;
    }

    return count;
}

void convertUtf8(unsigned char *utf8, short *buff) {
    while(*utf8)
        GET_UTF8_CHAR(utf8, *buff++);
}

int utf8Hash(unsigned char *utf8) {
    int hash = 0;

    while(*utf8) {
        short c;
        GET_UTF8_CHAR(utf8, c);
        hash = hash * 37 + c;
    }

    return hash;
}

int utf8Comp(unsigned char *ptr, unsigned char *ptr2) {
    while(*ptr && *ptr2) {
        short c, c2;

        GET_UTF8_CHAR(ptr, c);
        GET_UTF8_CHAR(ptr2, c2);
        if(c != c2)
            return FALSE;
    }
    if(*ptr || *ptr2)
        return FALSE;

    return TRUE;
}

unsigned char *findUtf8String(unsigned char *string) {
    unsigned char *interned;

    /* Add if absent, no scavenge, locked */
    findHashEntry(hash_table, string, interned, TRUE, FALSE, TRUE);

    return interned;
}

unsigned char *slash2dots(unsigned char *utf8) {
    int len = utf8Len(utf8);
    unsigned char *conv = (unsigned char*)sysMalloc(len+1);
    int i;

    for(i = 0; i <= len; i++)
        if(utf8[i] == '/')
            conv[i] = '.';
        else
            conv[i] = utf8[i];

    return conv;
}

void initialiseUtf8() {
    /* Init hash table, and create lock */
    initHashTable(hash_table, HASHTABSZE, TRUE);
}

#ifndef NO_JNI
/* Functions used by JNI */

int utf8CharLen(short *unicode, int len) {
    int count = 1;

    for(; len > 0; len--) {
        short c = *unicode++;
        count += c > 0x7f ? (c > 0x7ff ? 3 : 2) : 1;
    }

    return count;
}

char *unicode2Utf8(short *unicode, int len, char *utf8) {
    char *ptr = utf8;

    for(; len > 0; len--) {
        short c = *unicode++;
        if((c == 0) || (c > 0x7f)) {
            if(c > 0x7ff) {
                *ptr++ = (c >> 12) | 0xe0;
                *ptr++ = ((c >> 6) & 0x3f) | 0x80;
            } else
                *ptr++ = (c >> 6) | 0xc0;
            *ptr++ = (c&0x3f) | 0x80;
        } else
            *ptr++ = c;
    }

    *ptr = '\0';
    return utf8;
}
#endif
