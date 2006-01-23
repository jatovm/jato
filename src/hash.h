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

#include "thread.h"

typedef struct hash_entry {
    int hash;
    void *data;
} HashEntry;

typedef struct hash_table {
    HashEntry *hash_table;
    int hash_size;
    int hash_count;
    VMLock lock;
} HashTable;

extern void resizeHash(HashTable *table, int new_size);

#define initHashTable(table, initial_size, create_lock)                            \
{                                                                                  \
    table.hash_table = (HashEntry*)sysMalloc(sizeof(HashEntry)*initial_size);      \
    memset(table.hash_table, 0, sizeof(HashEntry)*initial_size);                   \
    table.hash_size = initial_size;                                                \
    table.hash_count = 0;                                                          \
    if(create_lock)                                                                \
        initVMLock(table.lock);                                                    \
}

#define lockHashTable(table)                                                       \
{                                                                                  \
    Thread *self = threadSelf();                                                   \
    disableSuspend(self);                                                          \
    lockVMLock(table.lock, self);                                                  \
}

#define unlockHashTable(table)                                                     \
{                                                                                  \
    Thread *self = threadSelf();                                                   \
    unlockVMLock(table.lock, self);                                                \
    enableSuspend(self);                                                           \
}

#define findHashEntry(table, ptr, ptr2, add_if_absent, scavenge, locked)           \
{                                                                                  \
    int hash = HASH(ptr);                                                          \
    int i;                                                                         \
                                                                                   \
    Thread *self;                                                                  \
    if(locked) {                                                                   \
        self = threadSelf();                                                       \
        if(!tryLockVMLock(table.lock, self)) {                                     \
            disableSuspend(self);                                                  \
            lockVMLock(table.lock, self);                                          \
            enableSuspend(self);                                                   \
        }                                                                          \
        fastDisableSuspend(self);                                                  \
    }                                                                              \
                                                                                   \
    i = hash & (table.hash_size - 1);                                              \
                                                                                   \
    for(;;) {                                                                      \
        ptr2 = table.hash_table[i].data;                                           \
        if((ptr2 == NULL) || (COMPARE(ptr, ptr2, hash, table.hash_table[i].hash))) \
            break;                                                                 \
                                                                                   \
        i = (i+1) & (table.hash_size - 1);                                         \
    }                                                                              \
                                                                                   \
    if(ptr2) {                                                                     \
        FOUND(ptr2);                                                               \
    } else                                                                         \
        if(add_if_absent) {                                                        \
            table.hash_table[i].hash = hash;                                       \
            ptr2 = table.hash_table[i].data = PREPARE(ptr);                        \
                                                                                   \
            table.hash_count++;                                                    \
            if((table.hash_count * 4) > (table.hash_size * 3)) {                   \
                int new_size;                                                      \
                if(scavenge) {                                                     \
                    int n;                                                         \
                    for(i = 0, n = table.hash_count; n--; i++) {                   \
                        void *data = table.hash_table[i].data;                     \
                        if(data && SCAVENGE(data)) {                               \
                            table.hash_table[i].data = NULL;                       \
                            table.hash_count--;                                    \
                        }                                                          \
                    }                                                              \
                    if((table.hash_count * 3) > (table.hash_size * 2))             \
                        new_size = table.hash_size*2;                              \
                    else                                                           \
                        new_size = table.hash_size;                                \
                } else                                                             \
                    new_size = table.hash_size*2;                                  \
                                                                                   \
                resizeHash(&table, new_size);                                      \
            }                                                                      \
        }                                                                          \
                                                                                   \
    if(locked) {                                                                   \
        fastEnableSuspend(self);                                                   \
        unlockVMLock(table.lock, self);                                            \
    }                                                                              \
}

#define hashIterate(table)                                                         \
{                                                                                  \
    int i;                                                                         \
                                                                                   \
    for(i = table.hash_size-1; i >= 0; i--) {                                      \
        void *data = table.hash_table[i].data;                                     \
        if(data)                                                                   \
            ITERATE(data);                                                         \
    }                                                                              \
}

#define freeHashTable(table)                                                       \
    free(table.hash_table);

