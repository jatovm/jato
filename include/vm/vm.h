/*
 * Copyright (C) 2003, 2004, 2005, 2006 Robert Lougher <rob@lougher.demon.co.uk>.
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

#ifndef __VM_H
#define __VM_H

#include <stdarg.h>
#include <inttypes.h>

/* Architecture dependent definitions */
#include <vm/arch.h>

/* Configure options */
#include <vm/config.h>

#ifndef TRUE
#define         TRUE    1
#define         FALSE   0
#endif

/* These should go in the interpreter file */

#define OPC_NOP                         0
#define OPC_ACONST_NULL                 1
#define OPC_ICONST_M1                   2
#define OPC_ICONST_0                    3
#define OPC_ICONST_1                    4
#define OPC_ICONST_2                    5
#define OPC_ICONST_3                    6
#define OPC_ICONST_4                    7
#define OPC_ICONST_5                    8
#define OPC_LCONST_0                    9
#define OPC_LCONST_1                    10
#define OPC_FCONST_0                    11
#define OPC_FCONST_1                    12
#define OPC_FCONST_2                    13
#define OPC_DCONST_0                    14
#define OPC_DCONST_1                    15
#define OPC_BIPUSH                      16
#define OPC_SIPUSH                      17
#define OPC_LDC                         18
#define OPC_LDC_W                       19
#define OPC_LDC2_W                      20
#define OPC_ILOAD                       21
#define OPC_LLOAD                       22
#define OPC_FLOAD                       23
#define OPC_DLOAD                       24
#define OPC_ALOAD                       25
#define OPC_ILOAD_0                     26
#define OPC_ILOAD_1                     27
#define OPC_ILOAD_2                     28
#define OPC_ILOAD_3                     29
#define OPC_LLOAD_0                     30
#define OPC_LLOAD_1                     31
#define OPC_LLOAD_2                     32
#define OPC_LLOAD_3                     33
#define OPC_FLOAD_0                     34
#define OPC_FLOAD_1                     35
#define OPC_FLOAD_2                     36
#define OPC_FLOAD_3                     37
#define OPC_DLOAD_0                     38
#define OPC_DLOAD_1                     39
#define OPC_DLOAD_2                     40
#define OPC_DLOAD_3                     41
#define OPC_ALOAD_0                     42
#define OPC_ALOAD_1                     43
#define OPC_ALOAD_2                     44
#define OPC_ALOAD_3                     45
#define OPC_IALOAD                      46
#define OPC_LALOAD                      47
#define OPC_FALOAD                      48
#define OPC_DALOAD                      49
#define OPC_AALOAD                      50
#define OPC_BALOAD                      51
#define OPC_CALOAD                      52
#define OPC_SALOAD                      53
#define OPC_ISTORE                      54
#define OPC_LSTORE                      55
#define OPC_FSTORE                      56
#define OPC_DSTORE                      57
#define OPC_ASTORE                      58
#define OPC_ISTORE_0                    59
#define OPC_ISTORE_1                    60
#define OPC_ISTORE_2                    61
#define OPC_ISTORE_3                    62
#define OPC_LSTORE_0                    63
#define OPC_LSTORE_1                    64
#define OPC_LSTORE_2                    65
#define OPC_LSTORE_3                    66
#define OPC_FSTORE_0                    67
#define OPC_FSTORE_1                    68
#define OPC_FSTORE_2                    69
#define OPC_FSTORE_3                    70
#define OPC_DSTORE_0                    71
#define OPC_DSTORE_1                    72
#define OPC_DSTORE_2                    73
#define OPC_DSTORE_3                    74
#define OPC_ASTORE_0                    75
#define OPC_ASTORE_1                    76
#define OPC_ASTORE_2                    77
#define OPC_ASTORE_3                    78
#define OPC_IASTORE                     79
#define OPC_LASTORE                     80
#define OPC_FASTORE                     81
#define OPC_DASTORE                     82
#define OPC_AASTORE                     83
#define OPC_BASTORE                     84
#define OPC_CASTORE                     85
#define OPC_SASTORE                     86
#define OPC_POP                         87
#define OPC_POP2                        88
#define OPC_DUP                         89
#define OPC_DUP_X1                      90
#define OPC_DUP_X2                      91
#define OPC_DUP2                        92
#define OPC_DUP2_X1                     93
#define OPC_DUP2_X2                     94
#define OPC_SWAP                        95
#define OPC_IADD                        96
#define OPC_LADD                        97
#define OPC_FADD                        98
#define OPC_DADD                        99
#define OPC_ISUB                        100
#define OPC_LSUB                        101
#define OPC_FSUB                        102
#define OPC_DSUB                        103
#define OPC_IMUL                        104
#define OPC_LMUL                        105
#define OPC_FMUL                        106
#define OPC_DMUL                        107
#define OPC_IDIV                        108
#define OPC_LDIV                        109
#define OPC_FDIV                        110
#define OPC_DDIV                        111
#define OPC_IREM                        112
#define OPC_LREM                        113
#define OPC_FREM                        114
#define OPC_DREM                        115
#define OPC_INEG                        116
#define OPC_LNEG                        117
#define OPC_FNEG                        118
#define OPC_DNEG                        119
#define OPC_ISHL                        120
#define OPC_LSHL                        121
#define OPC_ISHR                        122
#define OPC_LSHR                        123
#define OPC_IUSHR                       124
#define OPC_LUSHR                       125
#define OPC_IAND                        126
#define OPC_LAND                        127
#define OPC_IOR                         128     
#define OPC_LOR                         129     
#define OPC_IXOR                        130     
#define OPC_LXOR                        131     
#define OPC_IINC                        132
#define OPC_I2L                         133
#define OPC_I2F                         134
#define OPC_I2D                         135
#define OPC_L2I                         136
#define OPC_L2F                         137
#define OPC_L2D                         138
#define OPC_F2I                         139
#define OPC_F2L                         140
#define OPC_F2D                         141
#define OPC_D2I                         142
#define OPC_D2L                         143
#define OPC_D2F                         144
#define OPC_I2B                         145
#define OPC_I2C                         146
#define OPC_I2S                         147
#define OPC_LCMP                        148
#define OPC_FCMPL                       149
#define OPC_FCMPG                       150
#define OPC_DCMPL                       151
#define OPC_DCMPG                       152
#define OPC_IFEQ                        153
#define OPC_IFNE                        154
#define OPC_IFLT                        155
#define OPC_IFGE                        156
#define OPC_IFGT                        157
#define OPC_IFLE                        158
#define OPC_IF_ICMPEQ                   159
#define OPC_IF_ICMPNE                   160
#define OPC_IF_ICMPLT                   161
#define OPC_IF_ICMPGE                   162
#define OPC_IF_ICMPGT                   163
#define OPC_IF_ICMPLE                   164
#define OPC_IF_ACMPEQ                   165
#define OPC_IF_ACMPNE                   166
#define OPC_GOTO                        167
#define OPC_JSR                         168
#define OPC_RET                         169
#define OPC_TABLESWITCH                 170
#define OPC_LOOKUPSWITCH                171
#define OPC_IRETURN                     172
#define OPC_LRETURN                     173
#define OPC_FRETURN                     174
#define OPC_DRETURN                     175
#define OPC_ARETURN                     176
#define OPC_RETURN                      177
#define OPC_GETSTATIC                   178
#define OPC_PUTSTATIC                   179
#define OPC_GETFIELD                    180
#define OPC_PUTFIELD                    181
#define OPC_INVOKEVIRTUAL               182
#define OPC_INVOKESPECIAL               183
#define OPC_INVOKESTATIC                184
#define OPC_INVOKEINTERFACE             185
#define OPC_NEW                         187
#define OPC_NEWARRAY                    188
#define OPC_ANEWARRAY                   189
#define OPC_ARRAYLENGTH                 190
#define OPC_ATHROW                      191
#define OPC_CHECKCAST                   192
#define OPC_INSTANCEOF                  193
#define OPC_MONITORENTER                194
#define OPC_MONITOREXIT                 195
#define OPC_WIDE                        196
#define OPC_MULTIANEWARRAY              197
#define OPC_IFNULL                      198
#define OPC_IFNONNULL                   199
#define OPC_GOTO_W                      200
#define OPC_JSR_W                       201
#define OPC_LDC_QUICK                   203
#define OPC_LDC_W_QUICK                 204
#define OPC_GETFIELD_QUICK              206
#define OPC_PUTFIELD_QUICK              207
#define OPC_GETFIELD2_QUICK             208
#define OPC_PUTFIELD2_QUICK             209
#define OPC_GETSTATIC_QUICK             210
#define OPC_PUTSTATIC_QUICK             211
#define OPC_GETSTATIC2_QUICK            212
#define OPC_PUTSTATIC2_QUICK            213
#define OPC_INVOKEVIRTUAL_QUICK         214
#define OPC_INVOKENONVIRTUAL_QUICK      215
#define OPC_INVOKESUPER_QUICK           216
#define OPC_INVOKEVIRTUAL_QUICK_W       226
#define OPC_GETFIELD_QUICK_W            227
#define OPC_PUTFIELD_QUICK_W            228
#define OPC_GETFIELD_THIS               229
#define OPC_LOCK                        230
#define OPC_ALOAD_THIS                  231
#define OPC_INVOKESTATIC_QUICK          232
#define OPC_NEW_QUICK                   233
#define OPC_ANEWARRAY_QUICK             235
#define OPC_CHECKCAST_QUICK             238
#define OPC_INSTANCEOF_QUICK            239
#define OPC_MULTIANEWARRAY_QUICK        243
#define OPC_INVOKEINTERFACE_QUICK       244

#define CONSTANT_Utf8                   1
#define CONSTANT_Integer                3
#define CONSTANT_Float                  4
#define CONSTANT_Long                   5
#define CONSTANT_Double                 6
#define CONSTANT_Class                  7
#define CONSTANT_String                 8
#define CONSTANT_Fieldref               9
#define CONSTANT_Methodref              10
#define CONSTANT_InterfaceMethodref     11
#define CONSTANT_NameAndType            12

#define CONSTANT_Resolved               20
#define CONSTANT_Locked                 21

#define ACC_PUBLIC              0x0001
#define ACC_PRIVATE             0x0002
#define ACC_PROTECTED           0x0004
#define ACC_STATIC              0x0008
#define ACC_FINAL               0x0010
#define ACC_SYNCHRONIZED        0x0020
#define ACC_SUPER               0x0020
#define ACC_VOLATILE            0x0040
#define ACC_TRANSIENT           0x0080
#define ACC_NATIVE              0x0100
#define ACC_INTERFACE           0x0200
#define ACC_ABSTRACT            0x0400
#define ACC_SYNTHETIC           0x1000
#define ACC_ANNOTATION          0x2000
#define ACC_ENUM                0x4000
#define ACC_MIRANDA             0x0800

#define T_BOOLEAN               4
#define T_CHAR                  5       
#define T_FLOAT                 6
#define T_DOUBLE                7
#define T_BYTE                  8
#define T_SHORT                 9
#define T_INT                   10
#define T_LONG                  11

/* Class states */

#define CLASS_LOADED            1
#define CLASS_LINKED            2
#define CLASS_BAD               3
#define CLASS_INITING           4
#define CLASS_INITED            5

#define CLASS_ARRAY             6
#define CLASS_PRIM              7

/* Class flags */

#define REFERENCE               1
#define SOFT_REFERENCE          2
#define WEAK_REFERENCE          4
#define PHANTOM_REFERENCE       8
#define FINALIZED               16 
#define CLASS_LOADER            32
#define CLASS_CLASH             64
#define VMTHROWABLE             128

typedef unsigned char           u1;
typedef unsigned short          u2;
typedef unsigned int            u4;
typedef unsigned long long      u8;

typedef uintptr_t ConstantPoolEntry;

typedef struct constant_pool {
    volatile u1 *type;
    ConstantPoolEntry *info;
} ConstantPool;

typedef struct exception_table_entry {
    u2 start_pc;
    u2 end_pc;
    u2 handler_pc;
    u2 catch_type;
} ExceptionTableEntry;

typedef struct line_no_table_entry {
    u2 start_pc;
    u2 line_no;
} LineNoTableEntry;

typedef struct class {
   uintptr_t lock;
   struct class *class;
} Class;

typedef struct object {
   uintptr_t lock;
   struct class *class;
} Object;

#ifdef DIRECT
typedef union ins_operand {
    uintptr_t u;
    int i;
    struct {
        signed short i1;
        signed short i2;
    } ii;
    struct {
        unsigned short u1;
        unsigned short u2;
    } uu;
    struct {
        unsigned short u1;
        unsigned char u2;
        char i;
    } uui;
    void *pntr;
} Operand;

typedef struct instruction {
#ifdef DIRECT_DEBUG
    unsigned char opcode;
    char cache_depth;
    short bytecode_pc;
#endif
    const void *handler;
    Operand operand;
} Instruction;

typedef struct switch_table {
    int low;
    int high;
    Instruction *deflt;
    Instruction **entries;
} SwitchTable;

typedef struct lookup_entry {
    int key;
    Instruction *handler;
} LookupEntry;

typedef struct lookup_table {
    int num_entries;
    Instruction *deflt;
    LookupEntry *entries;
} LookupTable;

typedef Instruction *CodePntr;
#else
typedef unsigned char *CodePntr;
#endif

typedef struct methodblock {
   Class *class;
   char *name;
   char *type;
   u2 access_flags;
   u2 max_stack;
   u2 max_locals;
   u2 args_count;
   u2 throw_table_size;
   u2 exception_table_size;
   u2 line_no_table_size;
   int native_extra_arg;
   void *native_invoker;
   void *code;
   int code_size;
   u2 *throw_table;
   ExceptionTableEntry *exception_table;
   LineNoTableEntry *line_no_table;
   int method_table_index;
} MethodBlock;

typedef struct fieldblock {
   Class *class;
   char *name;
   char *type;
   u2 access_flags;
   u2 constant;
   uintptr_t static_value;
   u4 offset;
} FieldBlock;

typedef struct itable_entry {
   Class *interface;
   int *offsets;
} ITableEntry;

typedef struct refs_offsets_entry {
    int start;
    int end;
} RefsOffsetsEntry;

#define CLASS_PAD_SIZE 4

typedef struct classblock {
   uintptr_t pad[CLASS_PAD_SIZE];
   char *name;
   char *super_name;
   char *source_file_name;
   Class *super;
   u1 state;
   u1 flags;
   u2 access_flags;
   u2 interfaces_count;
   u2 fields_count;
   u2 methods_count;
   u2 constant_pool_count;
   int object_size;
   FieldBlock *fields;
   MethodBlock *methods;
   Class **interfaces;
   ConstantPool constant_pool;
   int method_table_size;
   MethodBlock **method_table;
   int imethod_table_size;
   ITableEntry *imethod_table;
   Class *element_class;
   int initing_tid;
   int dim;
   Object *class_loader;
   u2 declaring_class;
   u2 inner_access_flags;
   u2 inner_class_count;
   u2 *inner_classes;
   int refs_offsets_size;
   RefsOffsetsEntry *refs_offsets_table;
} ClassBlock;

typedef struct frame {
   MethodBlock *mb;
   CodePntr last_pc;
   uintptr_t *lvars;
   uintptr_t *ostack;
   struct frame *prev;
} Frame;

typedef struct jni_frame {
   MethodBlock *mb;
   Object **next_ref;
   Object **lrefs;
   uintptr_t *ostack;
   struct frame *prev;
} JNIFrame;

typedef struct exec_env {
    Object *exception;
    char *stack;
    char *stack_end;
    int stack_size;
    Frame *last_frame;
    Object *thread;
    char overflow;
} ExecEnv;

typedef struct prop {
    char *key;
    char *value;
} Property;

#define CLASS_CB(classRef)              ((ClassBlock*)(classRef+1))
#define INST_DATA(objectRef)            ((uintptr_t*)(objectRef+1))

#define ARRAY_DATA(arrayRef)            ((void*)(((u4*)(arrayRef+1))+1))
#define ARRAY_LEN(arrayRef)             *(u4*)(arrayRef+1)

#define IS_CLASS(object)                (!object->class || (object->class == java_lang_Class))

#define IS_INTERFACE(cb)                (cb->access_flags & ACC_INTERFACE)
#define IS_SYNTHETIC(cb)                (cb->access_flags & ACC_SYNTHETIC)
#define IS_ANNOTATION(cb)               (cb->access_flags & ACC_ANNOTATION)
#define IS_ENUM(cb)                     (cb->access_flags & ACC_ENUM)
#define IS_ARRAY(cb)                    (cb->state == CLASS_ARRAY)
#define IS_PRIMITIVE(cb)                (cb->state >= CLASS_PRIM)

#define IS_FINALIZED(cb)                (cb->flags & FINALIZED)
#define IS_REFERENCE(cb)		(cb->flags & REFERENCE)
#define IS_SOFT_REFERENCE(cb)		(cb->flags & SOFT_REFERENCE)
#define IS_WEAK_REFERENCE(cb)		(cb->flags & WEAK_REFERENCE)
#define IS_PHANTOM_REFERENCE(cb)	(cb->flags & PHANTOM_REFERENCE)
#define IS_CLASS_LOADER(cb)		(cb->flags & CLASS_LOADER)
#define IS_CLASS_DUP(cb)		(cb->flags & CLASS_CLASH)
#define IS_VMTHROWABLE(cb)		(cb->flags & VMTHROWABLE)

#define IS_SPECIAL(cb)			(cb->flags & (REFERENCE | CLASS_LOADER))

/* Macros for accessing constant pool entries */

#define CP_TYPE(cp,i)                   cp->type[i]
#define CP_INFO(cp,i)                   cp->info[i]
#define CP_CLASS(cp,i)                  (u2)cp->info[i]
#define CP_STRING(cp,i)                 (u2)cp->info[i]
#define CP_METHOD_CLASS(cp,i)           (u2)cp->info[i]
#define CP_METHOD_NAME_TYPE(cp,i)       (u2)(cp->info[i]>>16)
#define CP_INTERFACE_CLASS(cp,i)        (u2)cp->info[i]
#define CP_INTERFACE_NAME_TYPE(cp,i)    (u2)(cp->info[i]>>16)
#define CP_FIELD_CLASS(cp,i)            (u2)cp->info[i]
#define CP_FIELD_NAME_TYPE(cp,i)        (u2)(cp->info[i]>>16)
#define CP_NAME_TYPE_NAME(cp,i)         (u2)cp->info[i]
#define CP_NAME_TYPE_TYPE(cp,i)         (u2)(cp->info[i]>>16)
#define CP_UTF8(cp,i)                   (char *)(cp->info[i])

#define CP_INTEGER(cp,i)                (int)(cp->info[i])      
#define CP_FLOAT(cp,i)                  *(float *)&(cp->info[i])
#define CP_LONG(cp,i)                   *(long long *)&(cp->info[i])
#define CP_DOUBLE(cp,i)                 *(double *)&(cp->info[i])

#define KB 1024
#define MB (KB*KB)

/* minimum allowable size of object heap */
#define MIN_HEAP 4*KB

/* minimum allowable size of the Java stack */
#define MIN_STACK 2*KB

/* default minimum size of object heap */
#ifndef DEFAULT_MIN_HEAP
#define DEFAULT_MIN_HEAP 2*MB
#endif

/* default maximum size of object heap */
#ifndef DEFAULT_MAX_HEAP
#define DEFAULT_MAX_HEAP 128*MB
#endif

/* default size of the Java stack */
#define DEFAULT_STACK 64*KB

/* size of emergency area - big enough to create
   a StackOverflow exception */
#define STACK_RED_ZONE_SIZE 1*KB

/* --------------------- Function prototypes  --------------------------- */

/* Alloc */

extern void initialiseAlloc(unsigned long min, unsigned long max, int verbose);
extern void initialiseGC(int noasyncgc);
extern Class *allocClass();
extern Object *allocObject(Class *class);
extern Object *allocTypeArray(int type, int size);
extern Object *allocArray(Class *class, int size, int el_size);
extern Object *allocMultiArray(Class *array_class, int dim, intptr_t *count);
extern Object *cloneObject(Object *ob);
extern void markRoot(Object *ob);
extern void markObject(Object *ob, int mark, int mark_soft_refs);

extern void gc1();
extern void runFinalizers();

extern unsigned long freeHeapMem();
extern unsigned long totalHeapMem();
extern unsigned long maxHeapMem();

extern void *sysMalloc(int n);
extern void *sysRealloc(void *ptr, int n);

/* Class */

extern Class *java_lang_Class;

extern Class *defineClass(char *classname, char *data, int offset, int len, Object *class_loader);
extern void linkClass(Class *class);
extern Class *initClass(Class *class);
extern Class *findSystemClass(char *);
extern Class *findSystemClass0(char *);
extern Class *loadSystemClass(char *);

extern Class *findHashedClass(char *, Object *);
extern Class *findPrimitiveClass(char);
extern Class *findArrayClassFromClassLoader(char *, Object *);

extern Object *getSystemClassLoader();

extern int bootClassPathSize();
extern Object *bootClassPathResource(char *filename, int index);

#define findArrayClassFromClass(name, class) \
                    findArrayClassFromClassLoader(name, CLASS_CB(class)->class_loader)
#define findArrayClass(name) findArrayClassFromClassLoader(name, NULL)

extern Class *findClassFromClassLoader(char *, Object *);
#define findClassFromClass(name, class) \
                    findClassFromClassLoader(name, CLASS_CB(class)->class_loader)

extern void freeClassData(Class *class);
extern void freeClassLoaderData(Object *class_loader);

extern char *getClassPath();
extern char *getBootClassPath();
extern void initialiseClass(char *classpath, char *bootpath, char bootpathopt, int verbose);

/* resolve */

extern FieldBlock *findField(Class *, char *, char *);
extern MethodBlock *findMethod(Class *class, char *methodname, char *type);
extern FieldBlock *lookupField(Class *, char *, char *);
extern MethodBlock *lookupMethod(Class *class, char *methodname, char *type);
extern MethodBlock *lookupVirtualMethod(Object *ob, MethodBlock *mb);
extern Class *resolveClass(Class *class, int index, int init);
extern MethodBlock *resolveMethod(Class *class, int index);
extern MethodBlock *resolveInterfaceMethod(Class *class, int index);
extern FieldBlock *resolveField(Class *class, int index);
extern uintptr_t resolveSingleConstant(Class *class, int index);
extern int peekIsFieldLong(Class *class, int index);

/* cast */

extern char isSubClassOf(Class *class, Class *test);
extern char isInstanceOf(Class *class, Class *test);
extern char arrayStoreCheck(Class *class, Class *test);

/* execute */

extern void *executeMethodArgs(Object *ob, Class *class, MethodBlock *mb, ...);
extern void *executeMethodVaList(Object *ob, Class *class, MethodBlock *mb, va_list args);
extern void *executeMethodList(Object *ob, Class *class, MethodBlock *mb, u8 *args);

#define executeMethod(ob, mb, args...) \
    executeMethodArgs(ob, ob->class, mb, ##args)

#define executeStaticMethod(clazz, mb, args...) \
    executeMethodArgs(NULL, clazz, mb, ##args)

/* excep */

extern Object *exceptionOccured();
extern void signalChainedException(char *excep_name, char *excep_mess, Object *cause);
extern void setException(Object *excep);
extern void clearException();
extern void printException();
extern CodePntr findCatchBlock(Class *exception);
extern Object *setStackTrace();
extern Object *convertStackTrace(Object *vmthrwble);
extern int mapPC2LineNo(MethodBlock *mb, CodePntr pc_pntr);
extern void initialiseException();

#define exceptionOccured0(ee) \
    ee->exception

#define signalException(excep_name, excep_mess) \
    signalChainedException(excep_name, excep_mess, NULL)

/* interp */

extern uintptr_t *executeJava();
extern void initialiseInterpreter();

/* String */

extern Object *findInternedString(Object *string);
extern Object *createString(char *utf8);
extern Object *createStringFromUnicode(unsigned short *unicode, int len);
extern char *String2Cstr(Object *string);
extern int getStringLen(Object *string);
extern unsigned short *getStringChars(Object *string);
extern Object *getStringCharsArray(Object *string);
extern int getStringUtf8Len(Object *string);
extern char *String2Utf8(Object *string);
extern char *StringRegion2Utf8(Object *string, int start, int len, char *utf8);
extern void initialiseString();

#define Cstr2String(cstr) createString(cstr)

/* Utf8 */

extern int utf8Len(char *utf8);
extern int utf8Hash(char *utf8);
extern int utf8Comp(char *utf81, char *utf82);
extern void convertUtf8(char *utf8, unsigned short *buff);
extern char *findUtf8String(char *string);
extern int utf8CharLen(unsigned short *unicode, int len);
extern char *unicode2Utf8(unsigned short *unicode, int len, char *utf8);
extern char *slash2dots(char *utf8);
extern void initialiseUtf8();

/* Dll */

extern void *resolveNativeMethod(MethodBlock *mb);
extern int resolveDll(char *name);
extern char *getDllPath();
extern char *getBootDllPath();
extern char *getDllName(char *name);
extern void initialiseDll(int verbose);

extern uintptr_t *resolveNativeWrapper(Class *class, MethodBlock *mb, uintptr_t *ostack);

/* Dll OS */

extern char *nativeLibPath();
extern void *nativeLibOpen(char *path);
extern char *nativeLibMapName(char *name);
extern void *nativeLibSym(void *handle, char *symbol);

/* Threading */

extern void initialiseMainThread(int java_stack);
extern ExecEnv *getExecEnv();

extern void createJavaThread(Object *jThread, long long stack_size);
extern void mainThreadSetContextClassLoader(Object *loader);
extern void mainThreadWaitToExitVM();
extern void exitVM(int status);

/* Monitors */

extern void initialiseMonitor();

/* reflect */

extern Object *getClassConstructors(Class *class, int public);
extern Object *getClassMethods(Class *class, int public);
extern Object *getClassFields(Class *class, int public);
extern Object *getClassInterfaces(Class *class);
extern Object *getClassClasses(Class *class, int public);
extern Class *getDeclaringClass(Class *class);

extern Object *createWrapperObject(Class *type, uintptr_t *pntr);
extern uintptr_t *widenPrimitiveValue(int src_idx, int dest_idx, uintptr_t *src, uintptr_t *dest);
extern uintptr_t *unwrapAndWidenObject(Class *type, Object *arg, uintptr_t *pntr);
extern Object *invoke(Object *ob, MethodBlock *mb, Object *arg_array, Object *param_types,
                      int check_access);

extern MethodBlock *mbFromReflectObject(Object *reflect_ob);
extern FieldBlock *fbFromReflectObject(Object *reflect_ob);

extern Object *createReflectConstructorObject(MethodBlock *mb);
extern Object *createReflectMethodObject(MethodBlock *mb);
extern Object *createReflectFieldObject(FieldBlock *fb);
extern Class *getReflectMethodClass();

#define getPrimTypeIndex(cb) (cb->state - CLASS_PRIM)

/* jni */

extern void initialiseJNI();
extern void *getJNIInterface();

/* properties */

extern void addCommandLineProperties(Object *properties);
extern void addDefaultProperties(Object *properties);

/* access */

extern int checkClassAccess(Class *class1, Class *class2);
extern int checkMethodAccess(MethodBlock *mb, Class *class);
extern int checkFieldAccess(FieldBlock *fb, Class *class);

/* frame */

extern Frame *getCallerFrame(Frame *last);
extern Class *getCallerCallerClass();

/* native */

extern void initialiseNatives();

#endif
