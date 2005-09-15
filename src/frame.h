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

#define CREATE_TOP_FRAME(ee, class, mb, sp, ret)                \
{                                                               \
    Frame *last = ee->last_frame;                               \
    Frame *dummy = (Frame *)(last->ostack+last->mb->max_stack); \
    Frame *new_frame;                                           \
    u4 *new_ostack;                                             \
                                                                \
    ret = (void*) (sp = (u4*)(dummy+1));                        \
    new_frame = (Frame *)(sp + mb->max_locals);                 \
    new_ostack = (u4 *)(new_frame + 1);                         \
                                                                \
    if((char*)(new_ostack + mb->max_stack) > ee->stack_end) {   \
        if(ee->overflow++) {                                    \
            /* Overflow when we're already throwing stack       \
               overflow.  Stack extension should be enough      \
               to throw exception, so something's seriously     \
               gone wrong - abort the VM! */                    \
            printf("Fatal stack overflow!  Aborting VM.\n");    \
            exitVM(1);                                          \
        }                                                       \
        ee->stack_end += STACK_RED_ZONE_SIZE;                   \
        signalException("java/lang/StackOverflowError", NULL);  \
        return NULL;                                            \
    }                                                           \
                                                                \
    dummy->mb = NULL;                                           \
    dummy->ostack = sp;                                         \
    dummy->prev = last;                                         \
                                                                \
    new_frame->mb = mb;                                         \
    new_frame->lvars = sp;                                      \
    new_frame->ostack = new_ostack;                             \
                                                                \
    new_frame->prev = dummy;                                    \
    ee->last_frame = new_frame;                                 \
}

#define POP_TOP_FRAME(ee)                                       \
    ee->last_frame = ee->last_frame->prev->prev;
