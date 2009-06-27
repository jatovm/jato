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

#ifndef __VM_H
#define __VM_H

#include <stdarg.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>

/* Configure options */
#include <vm/config.h>

#include <vm/opcodes.h>

#include <jit/vtable.h>

#ifndef TRUE
#define         TRUE    1
#define         FALSE   0
#endif

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
#define CONSTANT_ResolvedClass          25
#define CONSTANT_ResolvedString         26
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
#define T_INT                  10
#define T_LONG                 11

/* Class states */

#define CLASS_LOADED            1
#define CLASS_LINKED            2
#define CLASS_BAD               3
#define CLASS_INITING           4
#define CLASS_INITED            5

#define CLASS_ARRAY             6
#define CLASS_PRIM              7

/* Class flags */

#define CLASS_CLASS             1
#define REFERENCE               2
#define SOFT_REFERENCE          4
#define WEAK_REFERENCE          8
#define PHANTOM_REFERENCE      16
#define FINALIZED              32 
#define CLASS_LOADER           64 
#define CLASS_CLASH           128
#define VMTHROWABLE           256 
#define ANONYMOUS             512
#define VMTHREAD             1024

typedef unsigned char           u1;
typedef unsigned short          u2;
typedef unsigned int            u4;
typedef unsigned long long      u8;

typedef char           s1;
typedef short          s2;
typedef int            s4;
typedef long long      s8;

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

typedef struct object Class;

typedef struct object {
   uintptr_t lock;
   Class *class;
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

typedef struct annotation_data {
   u1 *data;
   int len;
} AnnotationData;

typedef struct method_annotation_data {
    AnnotationData *annotations;
    AnnotationData *parameters;
    AnnotationData *dft_val;
} MethodAnnotationData;

struct compilation_unit;
struct jit_trampoline;

typedef struct methodblock {
   Class *class;
   char *name;
   char *type;
   char *signature;
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
   void *jit_code;
   int code_size;
   u2 *throw_table;
   ExceptionTableEntry *exception_table;
   LineNoTableEntry *line_no_table;
   int method_table_index;
   MethodAnnotationData *annotations;
   struct compilation_unit *compilation_unit;
   struct jit_trampoline *trampoline;
} MethodBlock;

typedef struct fieldblock {
   Class *class;
   char *name;
   char *type;
   char *signature;
   u2 access_flags;
   u2 constant;
   uintptr_t static_value;
   u4 offset;
   AnnotationData *annotations;
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
   struct vtable vtable;
   uintptr_t pad[CLASS_PAD_SIZE];
   char *name;
   char *signature;
   char *super_name;
   char *source_file_name;
   Class *super;
   u1 state;
   u2 flags;
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
   u2 enclosing_class;
   u2 enclosing_method;
   AnnotationData *annotations;
} ClassBlock;

typedef struct frame {
   CodePntr last_pc;
   uintptr_t *lvars;
   uintptr_t *ostack;
   MethodBlock *mb;
   struct frame *prev;
} Frame;

typedef struct jni_frame {
   Object **next_ref;
   Object **lrefs;
   uintptr_t *ostack;
   MethodBlock *mb;
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

typedef struct InitArgs {
    int noasyncgc;
    int verbosegc;
    int verbosedll;
    int verboseclass;

    int compact_specified; /* Whether compaction has been given on the
                              command line, and the value if it has */
    int do_compact;

    char *classpath;
    char *bootpath;
    char bootpathopt;

    int java_stack;
    unsigned long min_heap;
    unsigned long max_heap;

    Property *commandline_props;
    int props_count;

    void *main_stack_base;

    /* JNI invocation API hooks */
    
    int (*vfprintf)(FILE *stream, const char *fmt, va_list ap);
    void (*exit)(int status);
    void (*abort)(void);

} InitArgs;

#define CLASS_CB(classRef)              ((ClassBlock*)(classRef+1))
#define INST_DATA(objectRef)            ((uintptr_t*)(objectRef+1))

#define ARRAY_DATA(arrayRef)            ((void*)(((u4*)(arrayRef+1))+1))
#define ARRAY_LEN(arrayRef)             *(u4*)(arrayRef+1)

#define IS_CLASS(object)                (object->class && IS_CLASS_CLASS(CLASS_CB(object->class)))

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
#define IS_CLASS_CLASS(cb)		(cb->flags & CLASS_CLASS)
#define IS_VMTHROWABLE(cb)		(cb->flags & VMTHROWABLE)
#define IS_VMTHREAD(cb)			(cb->flags & VMTHREAD)
#define IS_ANONYMOUS(cb)		(cb->flags & ANONYMOUS)
#define IS_SPECIAL(cb)			(cb->flags & (REFERENCE | CLASS_LOADER | VMTHREAD))

#define IS_MEMBER(cb)			cb->declaring_class
#define IS_LOCAL(cb)			(cb->enclosing_method && !IS_ANONYMOUS(cb))

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

#define JAVA_COMPAT_VERSION "1.4.2"

/* --------------------- Function prototypes  --------------------------- */

/* Alloc */

extern void initialiseAlloc(InitArgs *args);
extern void initialiseGC(InitArgs *args);
extern Class *allocClass();
extern Object *allocObject(Class *class);
extern Object *allocTypeArray(int type, int size);
extern Object *allocArray(Class *class, int size, int el_size);
extern Object *allocMultiArray(Class *array_class, int dim, intptr_t *count);
extern Object *cloneObject(Object *ob);
extern void markRoot(Object *ob);
extern void markConservativeRoot(Object *ob);
extern void markObject(Object *ob, int mark, int mark_soft_refs);
extern uintptr_t getObjectHashcode(Object *ob);

extern void gc1();
extern void runFinalizers();

extern unsigned long freeHeapMem();
extern unsigned long totalHeapMem();
extern unsigned long maxHeapMem();

extern void *sysMalloc(int n);
extern void *sysRealloc(void *ptr, int n);

extern void registerStaticObjectRef(Object **ob);
extern void registerStaticObjectRefLocked(Object **ob);

#define registerStaticClassRef(ref) \
    registerStaticObjectRef(ref);

#define registerStaticClassRefLocked(ref) \
    registerStaticObjectRefLocked(ref);

/* GC support */
extern void threadReference(Object **ref);
extern int isMarked(Object *ob);

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

extern void markBootClasses();
extern void markLoaderClasses(Object *loader, int mark, int mark_soft_refs);
extern void threadBootClasses();
extern void threadLoaderClasses(Object *class_loader);
extern void initialiseClass(InitArgs *args);

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
extern Object *setStackTrace0(ExecEnv *ee, int max_depth);
extern Object *convertStackTrace(Object *vmthrwble);
extern int mapPC2LineNo(MethodBlock *mb, CodePntr pc_pntr);
extern void markVMThrowable(Object *vmthrwble, int mark, int mark_soft_refs);
extern void initialiseException();

#define exceptionOccured0(ee) \
    ee->exception

#define signalException(excep_name, excep_mess) \
    signalChainedException(excep_name, excep_mess, NULL)

#define setStackTrace() \
    setStackTrace0(getExecEnv(), INT_MAX)

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
extern void freeInternedStrings();
extern void threadInternedStrings();
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
extern int resolveDll(char *name, Object *loader);
extern char *getDllPath();
extern char *getBootDllPath();
extern char *getDllName(char *name);
extern void initialiseDll(InitArgs *args);
extern uintptr_t *resolveNativeWrapper(Class *class, MethodBlock *mb, uintptr_t *ostack);
extern void unloadClassLoaderDlls(Object *loader);
extern void threadLiveClassLoaderDlls();

/* Dll OS */

extern char *nativeLibPath();
extern void *nativeLibOpen(char *path);
extern void nativeLibClose(void *handle);
extern char *nativeLibMapName(char *name);
extern void *nativeLibSym(void *handle, char *symbol);
extern void *nativeStackBase();
extern int nativeAvailableProcessors();

/* Threading */

extern void initialiseMainThread(InitArgs *args);
extern ExecEnv *getExecEnv();

extern void createJavaThread(Object *jThread, long long stack_size);
extern void mainThreadSetContextClassLoader(Object *loader);
extern void mainThreadWaitToExitVM();
extern void exitVM(int status);
extern void scanThreads();

/* Monitors */

extern void initialiseMonitor();

/* reflect */

extern Object *getClassConstructors(Class *class, int public);
extern Object *getClassMethods(Class *class, int public);
extern Object *getClassFields(Class *class, int public);
extern Object *getClassInterfaces(Class *class);
extern Object *getClassClasses(Class *class, int public);
extern Class *getDeclaringClass(Class *class);
extern Class *getEnclosingClass(Class *class);
extern Object *getEnclosingMethodObject(Class *class);
extern Object *getEnclosingConstructorObject(Class *class);
extern Object *getClassAnnotations(Class *class);
extern Object *getFieldAnnotations(FieldBlock *fb);
extern Object *getMethodAnnotations(MethodBlock *mb);
extern Object *getMethodParameterAnnotations(MethodBlock *mb);
extern Object *getMethodDefaultValue(MethodBlock *mb);

extern Object *getReflectReturnObject(Class *type, uintptr_t *pntr);
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
extern void markJNIGlobalRefs();

/* properties */

extern void initialiseProperties(InitArgs *args);
extern void addCommandLineProperties(Object *properties);
extern void addDefaultProperties(Object *properties);
extern char *getCommandLineProperty(char *key);

/* access */

extern int checkClassAccess(Class *class1, Class *class2);
extern int checkMethodAccess(MethodBlock *mb, Class *class);
extern int checkFieldAccess(FieldBlock *fb, Class *class);

/* frame */

extern Frame *getCallerFrame(Frame *last);
extern Class *getCallerCallerClass();

/* native */

extern void initialiseNatives();

/* init */

extern void setDefaultInitArgs(InitArgs *args);
extern unsigned long parseMemValue(char *str);
extern void initVM(InitArgs *args);
extern int VMInitialising();

/* hooks */

extern void initialiseHooks(InitArgs *args);
extern void jam_fprintf(FILE *stream, const char *fmt, ...);
extern void jamvm_exit(int status);

#define jam_printf(fmt, ...) jam_fprintf(stdout, fmt, ## __VA_ARGS__)

#endif /* __VM_H */
