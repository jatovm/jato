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

#ifdef DIRECT
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include "jam.h"

#include "thread.h"
#include "interp.h"

#ifdef TRACEDIRECT
#define TRACE(x) printf x
#else
#define TRACE(x)
#endif

#define REWRITE_OPERAND(index) \
    operand.uui.u1 = index;    \
    operand.uui.u2 = opcode;   \
    operand.uui.i = ins_cache;

#define DEPTH_UNKNOWN -1

#define PREPARED   0
#define UNPREPARED 1
#define PREPARING  2

static VMWaitLock prepare_lock;

void initialiseDirect() {
    initVMWaitLock(prepare_lock);
}

void prepare(MethodBlock *mb, const void ***handlers) {
    int code_len = mb->code_size;
#ifdef USE_CACHE
    signed char cache_depth[code_len];
#endif
    Instruction *new_code;
    unsigned char *code;
    short map[code_len];
    int ins_count;
    int pass;
    int i;

    /* The bottom bits of the code pointer are used to
       indicate whether the method has been prepared.  The
       check in the interpreter is unsynchronised, so grab
       the lock and recheck.  If another thread tries to
       prepare the method, they will wait on the lock.  This
       lock is global, but methods are only prepared once,
       and contention to prepare a method is unlikely. */

    Thread *self = threadSelf();
    lockVMWaitLock(prepare_lock, self);

retry:
    code = mb->code;

    switch((unsigned int)code & 0x3) {
        case PREPARED:
            unlockVMWaitLock(prepare_lock, self);
            return;

        case UNPREPARED:
            mb->code = (void*)PREPARING;
            break;

        case PREPARING:
            waitVMWaitLock(prepare_lock, self);
            goto retry;
    }

    unlockVMWaitLock(prepare_lock, self);

    TRACE(("Preparing %s.%s%s\n", CLASS_CB(mb->class)->name, mb->name, mb->type));

    code--;

#ifdef USE_CACHE
    /* Initialise cache depth array, indicating that
       the depth of every bytecode is unknown */
    memset(cache_depth, DEPTH_UNKNOWN, code_len);

    /* Exception handlers are entered at cache-depth zero.  Initialise
       the start of each handler to trap "fall-through" into the handler
       -- this should never happen in code produced by javac */
    for(i = 0; i < mb->exception_table_size; i++)
        cache_depth[mb->exception_table[i].handler_pc] = 0;
#endif

    for(pass = 0; pass < 2; pass++) {
        int cache = 0;
        int pc;

        if(pass == 1)
            new_code = sysMalloc(ins_count * sizeof(Instruction));

        for(ins_count = 0, pc = 0; pc < code_len; ins_count++) {
            Operand operand;
            int ins_cache;
            int opcode;

#ifdef USE_CACHE
            /* If the instruction is reached via a branch (or catching an exception) the
               depth will already be known.  If this is different to the depth "falling
               through" we have a conflict.  Resolve it by inserting a NOP at the
               appropriate depth which re-caches.  This also handles dead-code.  Note,
               this generally only happens in the x ? y : z code sequence. */

            if((cache_depth[pc] != DEPTH_UNKNOWN) && (cache_depth[pc] != cache)) {
                TRACE(("CONFLICT @ addr: %d depth1 %d depth2 %d\n", pc, cache_depth[pc], cache));

                if(pass == 1)
                    new_code[ins_count].handler = (void**)handlers[cache][OPC_NOP];

                cache = 0;
                ins_count++;
            }
#endif
            /* Load the opcode -- we change it if it's WIDE, or ALOAD_0
               and the method is an instance method */
            opcode = code[pc];

            /* On pass one we calculate the cache depth and the mapping
               between bytecode and instruction numbering */
            if(pass == 0) {
#ifdef USE_CACHE
                TRACE(("%d : pc %d opcode %d cache %d\n", ins_count, pc, opcode, cache));
                cache_depth[pc] = cache;
#else
                TRACE(("%d : pc %d opcode %d\n", ins_count, pc, opcode));
#endif
                map[pc] = ins_count;
            }
#ifdef DIRECT_DEBUG
            else {
                new_code[ins_count].opcode = opcode;
                new_code[ins_count].bytecode_pc = pc;
                new_code[ins_count].cache_depth = cache;
            }
#endif
            /* For instructions without an operand */
            operand.i = 0;
            ins_cache = cache;

            switch(opcode) {
                default:
                    printf("Unrecognised bytecode %d found while preparing %s.%s%s\n",
                            opcode, CLASS_CB(mb->class)->name, mb->name, mb->type);
                    exitVM(1);

                case OPC_ALOAD_0:
#ifdef USE_CACHE
                    if(cache < 2) 
                        cache++;
#endif
                    /* If the next instruction is GETFIELD, and this is an instance method
                       rewrite it to ALOAD_THIS.  This will be rewritten in the interpreter
                       to GETFIELD_THIS */

                    if((code[++pc] == OPC_GETFIELD) && !(mb->access_flags & ACC_STATIC)) {
                        opcode = OPC_ALOAD_THIS;
                        REWRITE_OPERAND(cache);
                    } else
                        opcode = OPC_ILOAD_0;
                    break;

                case OPC_SIPUSH:
                    operand.i = READ_S2_OP(code + pc);
#ifdef USE_CACHE
                    if(cache < 2) 
                        cache++;
#endif
                    pc += 3;
                    break;
        
                case OPC_LDC_W:
                    REWRITE_OPERAND(READ_U2_OP(code + pc));
#ifdef USE_CACHE
                    if(cache < 2) 
                        cache++;
#endif
                    opcode = OPC_LDC;
                    pc += 3;
                    break;
    
                case OPC_LDC2_W:
                    operand.i = READ_U2_OP(code + pc);
#ifdef USE_CACHE
                    cache = 2;
#endif
                    pc += 3;
                    break;
        
                case OPC_LDC:
                    REWRITE_OPERAND(READ_U1_OP(code + pc));
#ifdef USE_CACHE
                    if(cache < 2) 
                        cache++;
#endif
                    pc += 2;
                    break;
    
                case OPC_BIPUSH:
                    operand.i = READ_S1_OP(code + pc);
#ifdef USE_CACHE
                    if(cache < 2) 
                        cache++;
#endif
                    pc += 2;
                    break;

                case OPC_ILOAD:
                case OPC_FLOAD:
                case OPC_ALOAD:
#ifdef USE_CACHE
                    operand.i = READ_U1_OP(code + pc);
                    if(cache < 2) 
                        cache++;
                    pc += 2;
                    break;
#endif
                case OPC_LLOAD:
                case OPC_DLOAD:
                    operand.i = READ_U1_OP(code + pc);
#ifdef USE_CACHE
                    cache = 2;
#endif
                    pc += 2;
                    break;
        
                case OPC_ACONST_NULL: case OPC_ICONST_M1: case OPC_ICONST_0:
                case OPC_FCONST_0: case OPC_ICONST_1: case OPC_ICONST_2:
                case OPC_ICONST_3: case OPC_ICONST_4: case OPC_ICONST_5:
                case OPC_FCONST_1: case OPC_FCONST_2: case OPC_ILOAD_0:
                case OPC_FLOAD_0: case OPC_ILOAD_1: case OPC_FLOAD_1:
                case OPC_ALOAD_1: case OPC_ILOAD_2: case OPC_FLOAD_2:
                case OPC_ALOAD_2: case OPC_ILOAD_3: case OPC_FLOAD_3:
                case OPC_ALOAD_3: case OPC_DUP:
#ifdef USE_CACHE
                    if(cache < 2) 
                        cache++;
                    pc += 1;
                    break;
#endif
                case OPC_LCONST_0: case OPC_DCONST_0: case OPC_DCONST_1:
                case OPC_LCONST_1: case OPC_LLOAD_0: case OPC_DLOAD_0:
                case OPC_LLOAD_1: case OPC_DLOAD_1: case OPC_LLOAD_2:
                case OPC_DLOAD_2: case OPC_LLOAD_3: case OPC_DLOAD_3:
                case OPC_LALOAD: case OPC_DALOAD: case OPC_DUP_X1:
                case OPC_DUP_X2: case OPC_DUP2: case OPC_DUP2_X1:
                case OPC_DUP2_X2: case OPC_SWAP: case OPC_LADD:
                case OPC_LSUB: case OPC_LMUL: case OPC_LDIV:
                case OPC_LREM: case OPC_LAND: case OPC_LOR:
                case OPC_LXOR: case OPC_LSHL: case OPC_LSHR:
                case OPC_LUSHR: case OPC_F2L: case OPC_D2L:
#ifdef USE_CACHE
                    cache = 2;
                    pc += 1;
                    break;
#endif
                case OPC_NOP: case OPC_LSTORE_0: case OPC_DSTORE_0:
                case OPC_LSTORE_1: case OPC_DSTORE_1: case OPC_LSTORE_2:
                case OPC_DSTORE_2: case OPC_LSTORE_3: case OPC_DSTORE_3:
                case OPC_IASTORE: case OPC_FASTORE: case OPC_AASTORE:
                case OPC_LASTORE: case OPC_DASTORE: case OPC_BASTORE:
                case OPC_CASTORE: case OPC_SASTORE: case OPC_POP2:
                case OPC_FADD: case OPC_DADD: case OPC_FSUB:
                case OPC_DSUB: case OPC_FMUL: case OPC_DMUL:
                case OPC_FDIV: case OPC_DDIV: case OPC_I2L:
                case OPC_I2F: case OPC_I2D: case OPC_L2I:
                case OPC_L2F: case OPC_L2D: case OPC_F2D:
                case OPC_D2F: case OPC_FREM: case OPC_DREM:
                case OPC_LNEG: case OPC_FNEG: case OPC_DNEG:
                case OPC_MONITORENTER: case OPC_MONITOREXIT:
#ifdef USE_CACHE
                    cache = 0;
                    pc += 1;
                    break;
#endif
                case OPC_ISTORE_0: case OPC_ASTORE_0: case OPC_FSTORE_0:
                case OPC_ISTORE_1: case OPC_ASTORE_1: case OPC_FSTORE_1:
                case OPC_ISTORE_2: case OPC_ASTORE_2: case OPC_FSTORE_2:
                case OPC_ISTORE_3: case OPC_ASTORE_3: case OPC_FSTORE_3:
                case OPC_POP:
#ifdef USE_CACHE
                    if(cache > 0)
                        cache--;
                    pc += 1;
                    break;
#endif
                case OPC_INEG:
#ifdef USE_CACHE
                    cache = cache == 2 ? 2 : 1;
                    pc += 1;
                    break;
#endif
                case OPC_ATHROW: case OPC_RETURN: case OPC_IRETURN:
                case OPC_ARETURN: case OPC_FRETURN: case OPC_LRETURN:
                case OPC_DRETURN:
#ifdef USE_CACHE
                    cache = 0;
                    pc += 1;
                    break;
#endif
                case OPC_IALOAD: case OPC_AALOAD: case OPC_FALOAD:
                case OPC_BALOAD: case OPC_CALOAD: case OPC_SALOAD:
                case OPC_IADD: case OPC_IMUL: case OPC_ISUB:
                case OPC_IDIV: case OPC_IREM: case OPC_IAND:
                case OPC_IOR: case OPC_IXOR: case OPC_F2I:
                case OPC_D2I: case OPC_I2B: case OPC_I2C:
                case OPC_I2S: case OPC_ISHL: case OPC_ISHR:
                case OPC_IUSHR: case OPC_LCMP: case OPC_DCMPG:
                case OPC_DCMPL: case OPC_FCMPG: case OPC_FCMPL:
                case OPC_ARRAYLENGTH:
#ifdef USE_CACHE
                    cache = 1;
#endif
                    pc += 1;
                    break;
    
                case OPC_LSTORE:
                case OPC_DSTORE:
#ifdef USE_CACHE
                    operand.i = READ_U1_OP(code + pc);
                    cache = 0;
                    pc += 2;
                    break;
#endif
                case OPC_ISTORE:
                case OPC_FSTORE:
                case OPC_ASTORE:
                    operand.i = READ_U1_OP(code + pc);
#ifdef USE_CACHE
                    if(cache > 0)
                        cache--;
#endif
                    pc += 2;
                    break;
        
                case OPC_IINC:
                    operand.ii.i1 = READ_U1_OP(code + pc); 
                    operand.ii.i2 = READ_S1_OP(code + pc + 1);
                    pc += 3;
                    break;
    
                case OPC_IFEQ: case OPC_IFNULL: case OPC_IFNE:
                case OPC_IFNONNULL: case OPC_IFLT: case OPC_IFGE:
                case OPC_IFGT: case OPC_IFLE: case OPC_IF_ACMPEQ:
                case OPC_IF_ICMPEQ: case OPC_IF_ACMPNE: case OPC_IF_ICMPNE:
                case OPC_IF_ICMPLT: case OPC_IF_ICMPGE: case OPC_IF_ICMPGT:
                case OPC_IF_ICMPLE:
                {
                    int dest = pc + READ_S2_OP(code + pc);
#ifdef USE_CACHE
                    /* Conflict can only occur on first pass, and dest must be backwards */
                    if(cache_depth[dest] > 0) {
                        TRACE(("CONFLICT in IF target addr: %d\n", dest));

                        /* Reset depthes calculated from the (backwards) destination and
                           here.  By setting depth at dest to zero (see below) and starting
                           from dest again, we will immediately get a conflict which will be
                           resolved by adding a NOP. */
                        memset(&cache_depth[dest + 1], DEPTH_UNKNOWN, pc - dest);
                        ins_count = map[dest];
                        cache = cache_depth[dest];
                        pc = dest;
                    } else {
                        cache = 0;
#endif
                        pc += 3;
#ifdef USE_CACHE
                    }
#endif
                    if(pass == 1) {
                        TRACE(("IF old dest %d new dest %d\n", dest, map[dest]));
                        operand.pntr = &new_code[map[dest]];
                    }
#ifdef USE_CACHE
                    /* Branches re-cache, so the cache depth at destination is zero */
                    else
                        cache_depth[dest] = 0;
#endif
                    break;
                }
    
                case OPC_PUTFIELD: case OPC_INVOKEVIRTUAL: case OPC_INVOKESPECIAL:
                case OPC_INVOKESTATIC: case OPC_CHECKCAST: case OPC_INSTANCEOF:
                    REWRITE_OPERAND(READ_U2_OP(code + pc));
#ifdef USE_CACHE
                    cache = 0;
#endif
                    pc += 3;
                    break;
        
                case OPC_GOTO_W:
                case OPC_GOTO:
                {
                    int delta, dest;
                    
                    if(opcode == OPC_GOTO)
                        delta = READ_S2_OP(code + pc);
                    else
                        delta = READ_S4_OP(code + pc);

                    dest = pc + delta;
#ifdef USE_CACHE
                    /* Conflict can only occur on first pass, and dest must be backwards */
                    if(cache_depth[dest] > 0) {
                        TRACE(("CONFLICT in GOTO target addr: %d\n", dest));

                        /* Reset depthes calculated from the (backwards) destination and
                           here.  By setting depth at dest to zero (see below) and starting
                           from dest again, we will immediately get a conflict which will be
                           resolved by adding a NOP. */
                        memset(&cache_depth[dest + 1], DEPTH_UNKNOWN, pc - dest);
                        ins_count = map[dest];
                        cache = cache_depth[dest];
                        pc = dest;
                    } else {
                        cache = 0;
#endif
                        pc += opcode == OPC_GOTO ? 3 : 5;
#ifdef USE_CACHE
                    }
#endif
                    /* GOTO re-caches, so the cache depth at destination is zero */
                    if(pass == 1) {
                        TRACE(("GOTO old dest %d new dest %d\n", dest, map[dest]));
                        operand.pntr = &new_code[map[dest]];
                    }
#ifdef USE_CACHE
                    else
                        cache_depth[dest] = 0;
#endif
                    break;
                }
        
                case OPC_TABLESWITCH:
                {
                    int *aligned_pc = (int*)(code + ((pc + 4) & ~0x3));
                    int deflt = ntohl(aligned_pc[0]);
                    int low   = ntohl(aligned_pc[1]);
                    int high  = ntohl(aligned_pc[2]);
                    int i;
    
                    if(pass == 0) {
#ifdef USE_CACHE
                        /* Destinations should always be forward WRT pc, i.e. not
                           visited yet.  Depths can only be -1 (or 0) so conflict
                           is impossible.  */

                        /* TABLESWITCH re-caches, so all possible destinations
                           are at cache depth zero */
                        cache_depth[pc + deflt] = 0;

                        for(i = 3; i < (high - low + 4); i++)
                            cache_depth[pc + ntohl(aligned_pc[i])] = 0;
#else
                        i = high - low + 4;
#endif
                    } else {
                        SwitchTable *table = (SwitchTable*) sysMalloc(sizeof(SwitchTable));

                        table->low = low;
                        table->high = high; 
                        table->deflt = &new_code[map[pc + deflt]];
                        table->entries = (Instruction**) sysMalloc((high - low + 1) * sizeof(Instruction *));

                        for(i = 3; i < (high - low + 4); i++)
                            table->entries[i - 3] = &new_code[map[pc + ntohl(aligned_pc[i])]];

                        operand.pntr = table;
                    }

                    pc = (unsigned char*)&aligned_pc[i] - code;
#ifdef USE_CACHE
                    cache = 0;
#endif    
                    break;
                }
        
                case OPC_LOOKUPSWITCH:
                {
                    int *aligned_pc = (int*)(code + ((pc + 4) & ~0x3));
                    int deflt  = ntohl(aligned_pc[0]);
                    int npairs = ntohl(aligned_pc[1]);
                    int i, j;
    
                    if(pass == 0) {
#ifdef USE_CACHE
                        /* Destinations should always be forward WRT pc, i.e. not
                           visited yet.  Depths can only be -1 (or 0) so conflict
                           is impossible.  */

                        /* LOOKUPSWITCH re-caches, so all possible destinations
                           are at cache depth zero */
                        cache_depth[pc + deflt] = 0;

                        for(i = 2; i < (npairs*2+2); i += 2)
                            cache_depth[pc + ntohl(aligned_pc[i+1])] = 0;
#else
                        i = npairs*2+2;
#endif
                    } else {
                        LookupTable *table = (LookupTable*)sysMalloc(sizeof(LookupTable));

                        table->num_entries = npairs;
                        table->deflt = &new_code[map[pc + deflt]];
                        table->entries = (LookupEntry*)sysMalloc(npairs * sizeof(LookupEntry));
                        
                        for(i = 2, j = 0; j < npairs; i += 2, j++) {
                            table->entries[j].key = ntohl(aligned_pc[i]);
                            table->entries[j].handler = &new_code[map[pc + ntohl(aligned_pc[i+1])]];
                        }
                        operand.pntr = table;
                    }

                    pc = (unsigned char*)&aligned_pc[i] - code;
#ifdef USE_CACHE
                    cache = 0;
#endif        
                    break;
                }
    
                case OPC_GETSTATIC:
#ifdef USE_CACHE
                {
                    int idx = READ_U2_OP(code + pc);
                    REWRITE_OPERAND(idx);

                    cache = cache > 0 || peekIsFieldLong(mb->class, idx) ? 2 : 1;
                    pc += 3;
                    break;
                }
#endif        
                case OPC_PUTSTATIC: 
#ifdef USE_CACHE
                {
                    int idx = READ_U2_OP(code + pc);
                    REWRITE_OPERAND(idx);

                    cache = cache < 2 || peekIsFieldLong(mb->class, idx) ? 0 : 1;
                    pc += 3;
                    break;
                }
#endif        
                case OPC_GETFIELD:
                {
                    int idx = READ_U2_OP(code + pc);
                    REWRITE_OPERAND(idx);
#ifdef USE_CACHE
                    cache = cache == 2 || peekIsFieldLong(mb->class, idx) ? 2 : 1;
#endif        
                    pc += 3;
                    break;
                }
    
                case OPC_INVOKEINTERFACE:
                    REWRITE_OPERAND(READ_U2_OP(code + pc));
#ifdef USE_CACHE
                    cache = 0;
#endif        
                    pc += 5;
                    break;
    
                case OPC_JSR_W:
                case OPC_JSR:
                {
                    int delta, dest;
                    
                    if(opcode == OPC_JSR)
                        delta = READ_S2_OP(code + pc);
                    else
                        delta = READ_S4_OP(code + pc);

                    dest = pc + delta;
#ifdef USE_CACHE
                    /* Conflict can only occur on first pass, and dest must be backwards */
                    if(cache_depth[dest] > 0) {
                        TRACE(("CONFLICT in JSR target addr: %d\n", dest));

                        /* Reset depthes calculated from the (backwards) destination and
                           here.  By setting depth at dest to zero (see below) and starting
                           from dest again, we will immediately get a conflict which will be
                           resolved by adding a NOP. */

                        memset(&cache_depth[dest + 1], DEPTH_UNKNOWN, pc - dest);
                        cache = cache_depth[dest];
                        ins_count = map[dest];
                        pc = dest;
                    } else {
                        cache = 0;
#endif
                        pc += opcode == OPC_JSR ? 3 : 5;
#ifdef USE_CACHE
                    }
#endif
                    if(pass == 1) {
                        TRACE(("JSR old dest %d new dest %d\n", dest, map[dest]));
                        operand.pntr = &new_code[map[dest]];
                    }
#ifdef USE_CACHE
                    /* JSR re-caches, so the cache depth at destination is zero */
                    else
                        cache_depth[dest] = 0;
#endif
                    break;
                }
    
                case OPC_RET:
#ifdef USE_CACHE
                    operand.i = READ_U1_OP(code + pc);
                    cache = 0;
                    pc += 2;
                    break;
#endif
                case OPC_NEWARRAY:
                    operand.i = READ_U1_OP(code + pc);
#ifdef USE_CACHE
                    cache = 1;
#endif
                    pc += 2;
                    break;
        
                case OPC_NEW:
                case OPC_ANEWARRAY:
                    REWRITE_OPERAND(READ_U2_OP(code + pc));
#ifdef USE_CACHE
                    cache = 1;
#endif
                    pc += 3;
                    break;
    
                case OPC_MULTIANEWARRAY:
                    operand.uui.u1 = READ_U2_OP(code + pc);
                    operand.uui.u2 = READ_U1_OP(code + pc + 2);
                    operand.uui.i = cache;
#ifdef USE_CACHE
                    cache = 1;
#endif
                    pc += 4;
                    break;
    
                /* All instructions are "wide" in the direct interpreter --
                   rewrite OPC_WIDE to the actual widened instruction */
                case OPC_WIDE:
                {
                   opcode = code[pc + 1];
        
                    switch(opcode) {
                        case OPC_ILOAD:
                        case OPC_FLOAD:
                        case OPC_ALOAD:
#ifdef USE_CACHE
                            operand.i = READ_U2_OP(code + pc + 1);
                            if(cache < 2) 
                                cache++;
                            pc += 4;
                            break;
#endif
                        case OPC_LLOAD:
                        case OPC_DLOAD:
#ifdef USE_CACHE
                            operand.i = READ_U2_OP(code + pc + 1);
                            cache = 2;
                            pc += 4;
                            break;
#endif
                        case OPC_ISTORE:
                        case OPC_FSTORE:
                        case OPC_ASTORE:
#ifdef USE_CACHE
                            operand.i = READ_U2_OP(code + pc + 1);
                            if(cache > 0)
                                cache--;
                            pc += 4;
                            break;
#endif
                        case OPC_LSTORE:
                        case OPC_DSTORE:
                        case OPC_RET:
                            operand.i = READ_U2_OP(code + pc + 1);
#ifdef USE_CACHE
                            cache = 0;
#endif
                            pc += 4;
                            break;
    
                        case OPC_IINC:
                            operand.ii.i1 = READ_U2_OP(code + pc + 1); 
                            operand.ii.i2 = READ_S2_OP(code + pc + 3);
                            pc += 6;
            		        break;
                    }
                }
            }

            /* Store the new instruction */
            if(pass == 1) {
                new_code[ins_count].handler = handlers[ins_cache][opcode];
                new_code[ins_count].operand = operand;
            }
        }
    }

    /* Update the method's line number and exception tables
      with the new instruction offsets */

    for(i = 0; i < mb->line_no_table_size; i++) {
        LineNoTableEntry *entry = &mb->line_no_table[i];
        entry->start_pc = map[entry->start_pc];
    }

    for(i = 0; i < mb->exception_table_size; i++) {
        ExceptionTableEntry *entry = &mb->exception_table[i];
        entry->start_pc = map[entry->start_pc];
        entry->end_pc = map[entry->end_pc];
        entry->handler_pc = map[entry->handler_pc];
    }

    /* Update the method with the new code.  This
       also marks the method as being prepared. */

    lockVMWaitLock(prepare_lock, self);
    mb->code = new_code;
    notifyAllVMWaitLock(prepare_lock, self);
    unlockVMWaitLock(prepare_lock, self);

    /* We don't need the old bytecode stream anymore */
    free(code);
}
#endif
