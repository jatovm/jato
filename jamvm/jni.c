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

#ifndef NO_JNI
#include <string.h>
#include <stdlib.h>
#include "jni.h"
#include "jam.h"
#include "thread.h"
#include "lock.h"

#define JNI_VERSION JNI_VERSION_1_4

/* The extra used in expanding the local refs table.
 * This must be >= size of JNIFrame to be thread safe
 * wrt GC thread suspension */
#define LIST_INCR 8

/* The amount of local reference space "ensured" before
   entering a JNI method.  The method is garaunteed to
   be able to create this amount without failure */
#define JNI_DEFAULT_LREFS 16

/* Forward declarations */
const jchar *Jam_GetStringChars(JNIEnv *env, jstring string, jboolean *isCopy);
void Jam_ReleaseStringChars(JNIEnv *env, jstring string, const jchar *chars);
jobject Jam_NewGlobalRef(JNIEnv *env, jobject obj);
void Jam_DeleteGlobalRef(JNIEnv *env, jobject obj);
JNIFrame *ensureJNILrefCapacity(int cap);
static void initJNIGrefs();

/* Cached values initialised on startup for JNI 1.4 NIO support */
static int buffCap_offset, buffAddr_offset, rawdata_offset;
static Class *buffImpl_class, *rawdata_class;
static MethodBlock *buffImpl_init_mb;
static char nio_init_OK = FALSE;

void initialiseJNI() {
    FieldBlock *buffCap_fb, *buffAddr_fb, *rawdata_fb;
    Class *buffer_class;

    /* Initialise the global references table */
    initJNIGrefs();

    /* Cache class and method/fields for JNI 1.4 NIO support */

    buffer_class = findSystemClass0("java/nio/Buffer");
    buffImpl_class = findSystemClass0("java/nio/DirectByteBufferImpl");
    rawdata_class = findSystemClass0(sizeof(uintptr_t) == 4 ? "gnu/classpath/Pointer32"
                                                            : "gnu/classpath/Pointer64");

    buffImpl_init_mb = findMethod(buffImpl_class, "<init>",
                      "(Ljava/lang/Object;Lgnu/classpath/Pointer;III)V");

    buffCap_fb = findField(buffer_class, "cap", "I");
    rawdata_fb = findField(rawdata_class, "data", sizeof(uintptr_t) == 4 ? "I" : "J");
    buffAddr_fb = findField(buffer_class, "address", "Lgnu/classpath/Pointer;");

    if(buffImpl_init_mb == NULL || buffCap_fb == NULL ||
             rawdata_fb == NULL || buffAddr_fb == NULL) {
        return;
    }

    registerStaticClassRef(&buffImpl_class);
    registerStaticClassRef(&rawdata_class);

    buffCap_offset = buffCap_fb->offset;
    buffAddr_offset = buffAddr_fb->offset;
    rawdata_offset = rawdata_fb->offset;
    nio_init_OK = TRUE;
}

int initJNILrefs() {
    JNIFrame *frame = ensureJNILrefCapacity(JNI_DEFAULT_LREFS);

    if(frame != NULL) {
        frame->next_ref = frame->lrefs + frame->mb->args_count;
        return TRUE;
    }

    return FALSE;
}

JNIFrame *expandJNILrefs(ExecEnv *ee, JNIFrame *frame, int incr) {
    JNIFrame *new_frame = (JNIFrame*)((Object**)frame + incr);

    if((char*)(new_frame + 1) > ee->stack_end)
        return NULL;

    memcpy(new_frame, frame, sizeof(JNIFrame));
    new_frame->ostack = (uintptr_t*)(new_frame + 1);
    ee->last_frame = (Frame*)new_frame;
    memset(frame, 0, incr * sizeof(Object*));
    return new_frame;
}

JNIFrame *ensureJNILrefCapacity(int cap) {
    ExecEnv *ee = getExecEnv();
    JNIFrame *frame = (JNIFrame*)ee->last_frame;
    int size = (Object**)frame - frame->lrefs - frame->mb->args_count;

    if(size < cap) {
        int incr = cap-size;
        if(incr < sizeof(JNIFrame)/sizeof(Object*))
            incr = sizeof(JNIFrame)/sizeof(Object*);

        if((frame = expandJNILrefs(ee, frame, incr)) == NULL)
            signalException("java/lang/OutOfMemoryError", "JNI local references");
    }

    return frame;
}

Object *addJNILref(Object *ref) {
    ExecEnv *ee = getExecEnv();
    JNIFrame *frame = (JNIFrame*)ee->last_frame;

    if(ref == NULL)
        return NULL;

    if(frame->next_ref == (Object**)frame)
        if((frame = expandJNILrefs(ee, frame, LIST_INCR)) == NULL) {
            jam_fprintf(stderr, "JNI - FatalError: cannot expand local references.\n");
            exitVM(1);
        }

    return *frame->next_ref++ = ref;
}

void delJNILref(Object *ref) {
    ExecEnv *ee = getExecEnv();
    JNIFrame *frame = (JNIFrame*)ee->last_frame;
    Object **opntr = frame->lrefs;

    for(; opntr < frame->next_ref; opntr++)
        if(*opntr == ref) {
            *opntr = NULL;
            return;
        }
}
 
JNIFrame *pushJNILrefFrame(int cap) {
    ExecEnv *ee = getExecEnv();
    JNIFrame *frame = (JNIFrame*)ee->last_frame;
    JNIFrame *new_frame = (JNIFrame*)((Object**)(frame + 1) + cap);

    if((char*)(new_frame + 1) > ee->stack_end) {
        signalException("java/lang/OutOfMemoryError", "JNI local references");
        return NULL;
    }

    new_frame->lrefs = new_frame->next_ref = (Object**)(frame + 1);
    new_frame->ostack = (uintptr_t*)(new_frame + 1);
    new_frame->prev = frame->prev;
    new_frame->mb = frame->mb;

    memset(frame + 1, 0, cap * sizeof(Object*));
    ee->last_frame = (Frame*)new_frame;

    return new_frame;
}

void popJNILrefFrame() {
    ExecEnv *ee = getExecEnv();
    JNIFrame *frame = (JNIFrame*)ee->last_frame;
    JNIFrame *prev = (JNIFrame*)frame->lrefs - 1;

    ee->last_frame = (Frame*)prev;
}

static VMLock global_ref_lock;
static Object **global_ref_table;
static int global_ref_size = 0;
static int global_ref_next = 0;
static int global_ref_deleted = 0;

static void initJNIGrefs() {
    initVMLock(global_ref_lock);
}

static Object *addJNIGref(Object *ref) {
    Thread *self = threadSelf();
    disableSuspend(self);
    lockVMLock(global_ref_lock, self);

    if(global_ref_next == global_ref_size) {
        int new_size;
        Object **new_table;
        int i, j;
               
        if(global_ref_deleted >= LIST_INCR) {
            new_size = global_ref_size;
            new_table = global_ref_table;
        } else {
            new_size = global_ref_size + LIST_INCR - global_ref_deleted;
            new_table = (Object**)sysMalloc(new_size * sizeof(Object*));
        }

        for(i = 0, j = 0; i < global_ref_size; i++)
            if(global_ref_table[i])
                new_table[j++] = global_ref_table[i];

        global_ref_next = j;
        global_ref_size = new_size;
        global_ref_table = new_table;
        global_ref_deleted = 0;
    }

    global_ref_table[global_ref_next++] = ref;

    unlockVMLock(global_ref_lock, self);
    enableSuspend(self);

    return ref;
}

static void delJNIGref(Object *ref) {
    Thread *self = threadSelf();
    int i;

    disableSuspend(self);
    lockVMLock(global_ref_lock, self);

    for(i = 0; i < global_ref_next; i++)
        if(global_ref_table[i] == ref) {
            global_ref_table[i] = NULL;
            global_ref_deleted++;
            break;
        }

    unlockVMLock(global_ref_lock, self);
    enableSuspend(self);
}

/* Called during mark phase of GC.  Safe from
   suspension but must grab lock to ensure list
   isn't being modified by another thread. */

void markJNIGlobalRefs() {
    Thread *self = threadSelf();
    int i;

    lockVMLock(global_ref_lock, self);

    for(i = 0; i < global_ref_next; i++)
        if(global_ref_table[i])
            markConservativeRoot(global_ref_table[i]);

    unlockVMLock(global_ref_lock, self);
}

/* Extensions added to JNI in JDK 1.4 */

jobject Jam_NewDirectByteBuffer(JNIEnv *env, void *addr, jlong capacity) {
    Object *buff, *rawdata;

    if(!nio_init_OK)
        return NULL;

    if((buff = allocObject(buffImpl_class)) != NULL &&
            (rawdata = allocObject(rawdata_class)) != NULL) {

        INST_DATA(rawdata)[rawdata_offset] = (uintptr_t)addr;
        executeMethod(buff, buffImpl_init_mb, NULL, rawdata, (int)capacity,
                      (int)capacity, 0);
    }

    return (jobject) addJNILref(buff);
}

static void *Jam_GetDirectBufferAddress(JNIEnv *env, jobject buffer) {
    Object *buff = (Object*)buffer;

    if(!nio_init_OK)
        return NULL;

    if(buff != NULL) {
        Object *rawdata = (Object *)INST_DATA(buff)[buffAddr_offset];
        if(rawdata != NULL)
            return (void*)INST_DATA(rawdata)[rawdata_offset];
    }

    return NULL;
}

jlong Jam_GetDirectBufferCapacity(JNIEnv *env, jobject buffer) {
    Object *buff = (Object*)buffer;

    if(!nio_init_OK)
        return -1;

    if(buff != NULL) {
        Object *rawdata = (Object *)INST_DATA(buff)[buffAddr_offset];
        if(rawdata != NULL)
            return (jlong)INST_DATA(buff)[buffCap_offset];
    }

    return -1;
}

/* Extensions added to JNI in JDK 1.2 */

jmethodID Jam_FromReflectedMethod(JNIEnv *env, jobject method) {
    return mbFromReflectObject((Object*)method);
}

jfieldID Jam_FromReflectedField(JNIEnv *env, jobject field) {
    return fbFromReflectObject((Object*)field);
}

jobject Jam_ToReflectedMethod(JNIEnv *env, jclass cls, jmethodID methodID, jboolean isStatic) {
    MethodBlock *mb = (MethodBlock *)methodID;
    if(strcmp(mb->name, "<init>") == 0)
        return (jobject) createReflectConstructorObject(mb);
    else
        return (jobject) createReflectMethodObject(mb);
}

jobject Jam_ToReflectedField(JNIEnv *env, jclass cls, jfieldID fieldID, jboolean isStatic) {
    return (jobject) createReflectFieldObject((FieldBlock *)fieldID);
}

jint Jam_PushLocalFrame(JNIEnv *env, jint capacity) {
    return pushJNILrefFrame(capacity) == NULL ? JNI_ERR : JNI_OK;
}

jobject Jam_PopLocalFrame(JNIEnv *env, jobject result) {
    popJNILrefFrame();
    return addJNILref(result);
}

jobject Jam_NewLocalRef(JNIEnv *env, jobject obj) {
    return addJNILref(obj);
}

jint Jam_EnsureLocalCapacity(JNIEnv *env, jint capacity) {
    return ensureJNILrefCapacity(capacity) == NULL ? JNI_ERR : JNI_OK;
}

void Jam_GetStringRegion(JNIEnv *env, jstring string, jsize start, jsize len, jchar *buf) {
    if((start + len) > getStringLen((Object*)string))
        signalException("java/lang/StringIndexOutOfBoundsException", NULL);
    else
        memcpy(buf, getStringChars((Object*)string) + start, len * sizeof(short));
}

void Jam_GetStringUTFRegion(JNIEnv *env, jstring string, jsize start, jsize len, char *buf) {
    if((start + len) > getStringLen((Object*)string))
        signalException("java/lang/StringIndexOutOfBoundsException", NULL);
    else
        StringRegion2Utf8((Object*)string, start, len, buf);
}

void *Jam_GetPrimitiveArrayCritical(JNIEnv *env, jarray array, jboolean *isCopy) {
    if(isCopy != NULL)
        *isCopy = JNI_FALSE;
    addJNIGref((Object*)array);
    return ARRAY_DATA((Object*)array);
}

void Jam_ReleasePrimitiveArrayCritical(JNIEnv *env, jarray array, void *carray, jint mode) {
    delJNIGref((Object*)array);
}

const jchar *Jam_GetStringCritical(JNIEnv *env, jstring string, jboolean *isCopy) {
    return Jam_GetStringChars(env, string, isCopy);
}

void Jam_ReleaseStringCritical(JNIEnv *env, jstring string, const jchar *chars) {
    Jam_ReleaseStringChars(env, string, chars);
}

jweak Jam_NewWeakGlobalRef(JNIEnv *env, jobject obj) {
    return Jam_NewGlobalRef(env, obj);
}

void Jam_DeleteWeakGlobalRef(JNIEnv *env, jweak obj) {
    Jam_DeleteGlobalRef(env, obj);
}

jboolean Jam_ExceptionCheck(JNIEnv *env) {
    return exceptionOccured() ? JNI_TRUE : JNI_FALSE;
}

/* JNI 1.1 interface */

jint Jam_GetVersion(JNIEnv *env) {
    return JNI_VERSION;
}

jclass Jam_DefineClass(JNIEnv *env, const char *name, jobject loader, const jbyte *buf, jsize bufLen) {
    return (jclass)defineClass((char*)name, (char *)buf, 0, (int)bufLen, (Object *)loader);
}

jclass Jam_FindClass(JNIEnv *env, const char *name) {
    /* We use the class loader associated with the calling native method.
       However, if this has been called from an attached thread there may
       be no native Java frame.  In this case use the system class loader */
    Frame *last = getExecEnv()->last_frame;
    Object *loader = last->prev ? CLASS_CB(last->mb->class)->class_loader
                                : getSystemClassLoader();

    return (jclass) findClassFromClassLoader((char*) name, loader);
}

jclass Jam_GetSuperClass(JNIEnv *env, jclass clazz) {
    ClassBlock *cb = CLASS_CB((Class *)clazz);
    return IS_INTERFACE(cb) ? NULL : (jclass)cb->super;
}

jboolean Jam_IsAssignableFrom(JNIEnv *env, jclass clazz1, jclass clazz2) {
    return isInstanceOf((Class*)clazz2, (Class*)clazz1);
}

jint Jam_Throw(JNIEnv *env, jthrowable obj) {
    Object *ob = (Object*)obj;
    setStackTrace();
    setException(ob);
    return JNI_TRUE;
}

jint Jam_ThrowNew(JNIEnv *env, jclass clazz, const char *message) {
    signalException(CLASS_CB((Class*)clazz)->name, (char*)message);
    return JNI_TRUE;
}

jthrowable Jam_ExceptionOccurred(JNIEnv *env) {
    return (jthrowable) addJNILref(exceptionOccured());
}

void Jam_ExceptionDescribe(JNIEnv *env) {
    printException();
}

void Jam_ExceptionClear(JNIEnv *env) {
    clearException();
}

void Jam_FatalError(JNIEnv *env, const char *message) {
    jam_fprintf(stderr, "JNI - FatalError: %s\n", message);
    exitVM(1);
}

jobject Jam_NewGlobalRef(JNIEnv *env, jobject obj) {
    return addJNIGref((Object*)obj);
}

void Jam_DeleteGlobalRef(JNIEnv *env, jobject obj) {
    delJNIGref((Object*)obj);
}

void Jam_DeleteLocalRef(JNIEnv *env, jobject obj) {
    delJNILref((Object*)obj);
}

jboolean Jam_IsSameObject(JNIEnv *env, jobject obj1, jobject obj2) {
    return obj1 == obj2;
}

jobject Jam_AllocObject(JNIEnv *env, jclass clazz) {
    return (jobject) addJNILref(allocObject((Class*)clazz));
}

jclass Jam_GetObjectClass(JNIEnv *env, jobject obj) {
    return (jobject)((Object*)obj)->class;
}

jboolean Jam_IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz) {
    return (obj == NULL) || isInstanceOf((Class*)clazz, ((Object*)obj)->class);
}

jmethodID Jam_GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    Class *class = initClass((Class *)clazz);
    MethodBlock *mb = lookupMethod(class, (char*)name, (char*)sig);

    if(mb == NULL)
        signalException("java/lang/NoSuchMethodError", (char *)name);

    return (jmethodID) mb;
}

jfieldID Jam_GetFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    Class *class = initClass((Class *)clazz);
    FieldBlock *fb = lookupField(class, (char*)name, (char*)sig);

    if(fb == NULL)
        signalException("java/lang/NoSuchFieldError", (char *)name);

    return (jfieldID) fb;
}

jmethodID Jam_GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    Class *class = initClass((Class *)clazz);
    MethodBlock *mb = findMethod(class, (char*)name, (char*)sig);

    if(mb == NULL)
        signalException("java/lang/NoSuchMethodError", (char *)name);

    return (jmethodID) mb;
}

jfieldID Jam_GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig) {
    Class *class = initClass((Class *)clazz);
    FieldBlock *fb = findField(class, (char*)name, (char*)sig);

    if(fb == NULL)
        signalException("java/lang/NoSuchFieldError", (char *)name);

    return (jfieldID) fb;
}

jstring Jam_NewString(JNIEnv *env, const jchar *unicodeChars, jsize len) {
    return (jstring) addJNILref(createStringFromUnicode((unsigned short*)unicodeChars, len));
}

jsize Jam_GetStringLength(JNIEnv *env, jstring string) {
    return getStringLen((Object*)string);
}

const jchar *Jam_GetStringChars(JNIEnv *env, jstring string, jboolean *isCopy) {
    if(isCopy != NULL)
        *isCopy = JNI_FALSE;

    /* Pin the reference */
    addJNIGref(getStringCharsArray((Object*)string));

    return (const jchar *)getStringChars((Object*)string);
}

void Jam_ReleaseStringChars(JNIEnv *env, jstring string, const jchar *chars) {
    /* Unpin the reference */
    delJNIGref(getStringCharsArray((Object*)string));
}

jstring Jam_NewStringUTF(JNIEnv *env, const char *bytes) {
    return (jstring) addJNILref(createString((char*)bytes));
}

jsize Jam_GetStringUTFLength(JNIEnv *env, jstring string) {
    if(string == NULL)
        return 0;
    return getStringUtf8Len((Object*)string);
}

const char *Jam_GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy) {
    if(isCopy != NULL)
        *isCopy = JNI_TRUE;

    if(string == NULL)
        return NULL;
    return (const char*)String2Utf8((Object*)string);
}

void Jam_ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *chars) {
    free((void*)chars);
}

jsize Jam_GetArrayLength(JNIEnv *env, jarray array) {
    return ARRAY_LEN((Object*)array);
}

jobject Jam_NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    Object *ob =  allocObject((Class*)clazz);

    if(ob) {
        va_list jargs;
        va_start(jargs, methodID);
        executeMethodVaList(ob, ob->class, (MethodBlock*)methodID, jargs);
        va_end(jargs);
    }

    return (jobject) addJNILref(ob);
}

jobject Jam_NewObjectA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *args) {
    Object *ob =  allocObject((Class*)clazz);

    if(ob) executeMethodList(ob, ob->class, (MethodBlock*)methodID, (u8*)args);
    return (jobject) addJNILref(ob);
}

jobject Jam_NewObjectV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args) {
    Object *ob =  allocObject((Class*)clazz);

    if(ob) executeMethodVaList(ob, ob->class, (MethodBlock*)methodID, args);
    return (jobject) addJNILref(ob);
}

jarray Jam_NewObjectArray(JNIEnv *env, jsize length, jclass elementClass, jobject initialElement) {
    char *element_name = CLASS_CB((Class*)elementClass)->name;
    char ac_name[strlen(element_name) + 4];
    Class *array_class;

    if(length < 0) {
        signalException("java/lang/NegativeArraySizeException", NULL);
        return NULL;
    }

    if(element_name[0] == '[')
        strcat(strcpy(ac_name, "["), element_name);
    else
        strcat(strcat(strcpy(ac_name, "[L"), element_name), ";");

    array_class = findArrayClassFromClass(ac_name, (Class*)elementClass);
    if(array_class != NULL) {
        Object *array = allocArray(array_class, length, sizeof(Object*));
        if(array != NULL) {
            if(initialElement != NULL) {
                Object **data = ARRAY_DATA(array);

                while(length--)
                   *data++ = (Object*) initialElement;
            }
            return (jarray) addJNILref(array);
        }
    }
    return NULL;
}

jarray Jam_GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index) {
    return (jarray) addJNILref(((Object**)ARRAY_DATA((Object*)array))[index]);
}

void Jam_SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index, jobject value) {
    ((Object**)ARRAY_DATA((Object*)array))[index] = (Object*)value;
}

jint Jam_RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods, jint nMethods) {
    return JNI_OK;
}

jint Jam_UnregisterNatives(JNIEnv *env, jclass clazz) {
    return JNI_OK;
}

jint Jam_MonitorEnter(JNIEnv *env, jobject obj) {
    objectLock((Object*)obj);
    return JNI_OK;
}

jint Jam_MonitorExit(JNIEnv *env, jobject obj) {
    objectUnlock((Object*)obj);
    return JNI_OK;
}

struct _JNIInvokeInterface Jam_JNIInvokeInterface;
JavaVM invokeIntf = &Jam_JNIInvokeInterface; 

jint Jam_GetJavaVM(JNIEnv *env, JavaVM **vm) {
    *vm = &invokeIntf;
    return JNI_OK;
}

#define GET_FIELD(type, native_type)                                                       \
native_type Jam_Get##type##Field(JNIEnv *env, jobject obj, jfieldID fieldID) {             \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    Object *ob = (Object*) obj;                                                            \
    return *(native_type *)&(INST_DATA(ob)[fb->offset]);                                   \
}

#define INT_GET_FIELD(type, native_type)                                                   \
native_type Jam_Get##type##Field(JNIEnv *env, jobject obj, jfieldID fieldID) {             \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    Object *ob = (Object*) obj;                                                            \
    return (native_type)(INST_DATA(ob)[fb->offset]);                                       \
}

#define SET_FIELD(type, native_type)                                                       \
void Jam_Set##type##Field(JNIEnv *env, jobject obj, jfieldID fieldID, native_type value) { \
    Object *ob = (Object*) obj;                                                            \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    *(native_type *)&(INST_DATA(ob)[fb->offset]) = value;                                  \
}

#define INT_SET_FIELD(type, native_type)                                                   \
void Jam_Set##type##Field(JNIEnv *env, jobject obj, jfieldID fieldID, native_type value) { \
    Object *ob = (Object*) obj;                                                            \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    INST_DATA(ob)[fb->offset] = (intptr_t)value;                                           \
}

#define GET_STATIC_FIELD(type, native_type)                                                \
native_type Jam_GetStatic##type##Field(JNIEnv *env, jclass clazz, jfieldID fieldID) {      \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    return *(native_type *)&fb->static_value;                                              \
}

#define INT_GET_STATIC_FIELD(type, native_type)                                            \
native_type Jam_GetStatic##type##Field(JNIEnv *env, jclass clazz, jfieldID fieldID) {      \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    return (native_type)fb->static_value;                                                  \
}

#define SET_STATIC_FIELD(type, native_type)                                                \
void Jam_SetStatic##type##Field(JNIEnv *env, jclass clazz, jfieldID fieldID,               \
                native_type value) {                                                       \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    *(native_type *)&fb->static_value = value;                                             \
}

#define INT_SET_STATIC_FIELD(type, native_type)                                            \
void Jam_SetStatic##type##Field(JNIEnv *env, jclass clazz, jfieldID fieldID,               \
                native_type value) {                                                       \
    FieldBlock *fb = (FieldBlock *) fieldID;                                               \
    fb->static_value = (intptr_t)value;                                                    \
}

#define FIELD_ACCESS(type, native_type)          \
        GET_FIELD(type, native_type);            \
        SET_FIELD(type, native_type);            \
        GET_STATIC_FIELD(type, native_type);     \
        SET_STATIC_FIELD(type, native_type);

#define INT_FIELD_ACCESS(type, native_type)      \
        INT_GET_FIELD(type, native_type);        \
        INT_SET_FIELD(type, native_type);        \
        INT_GET_STATIC_FIELD(type, native_type); \
        INT_SET_STATIC_FIELD(type, native_type);

INT_FIELD_ACCESS(Boolean, jboolean);
INT_FIELD_ACCESS(Byte, jbyte);
INT_FIELD_ACCESS(Char, jchar);
INT_FIELD_ACCESS(Short, jshort);
INT_FIELD_ACCESS(Int, jint);
FIELD_ACCESS(Long, jlong);
FIELD_ACCESS(Float, jfloat);
FIELD_ACCESS(Double, jdouble);

jobject Jam_GetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID) {
    FieldBlock *fb = (FieldBlock *) fieldID;
    Object *ob = (Object*) obj;
    return (jobject) addJNILref((Object*)(INST_DATA(ob)[fb->offset]));
}

void Jam_SetObjectField(JNIEnv *env, jobject obj, jfieldID fieldID, jobject value) {
    Object *ob = (Object*) obj;
    FieldBlock *fb = (FieldBlock *) fieldID;
    INST_DATA(ob)[fb->offset] = (uintptr_t)value;
}

jobject Jam_GetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID) {
    FieldBlock *fb = (FieldBlock *) fieldID;
    return (jobject) addJNILref((Object*)fb->static_value);
}

void Jam_SetStaticObjectField(JNIEnv *env, jclass clazz, jfieldID fieldID, jobject value) {
    FieldBlock *fb = (FieldBlock *) fieldID;
    fb->static_value = (uintptr_t)value;
}

#define VIRTUAL_METHOD(type, native_type)                                                        \
native_type Jam_Call##type##Method(JNIEnv *env, jobject obj, jmethodID mID, ...) {               \
    Object *ob = (Object *)obj;                                                                  \
    native_type *ret;                                                                            \
    va_list jargs;                                                                               \
                                                                                                 \
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)mID);                                \
    if(mb == NULL)                                                                               \
        return (native_type)0;                                                                   \
                                                                                                 \
    va_start(jargs, mID);                                                                        \
    ret = (native_type*) executeMethodVaList(ob, ob->class, mb, jargs);                          \
    va_end(jargs);                                                                               \
                                                                                                 \
    return *ret;                                                                                 \
}                                                                                                \
                                                                                                 \
native_type Jam_Call##type##MethodV(JNIEnv *env, jobject obj, jmethodID mID, va_list jargs) {    \
    Object *ob = (Object *)obj;                                                                  \
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)mID);                                \
    return mb == NULL ? (native_type)0 :                                                         \
                       *(native_type*)executeMethodVaList(ob, ob->class, mb, jargs);             \
}                                                                                                \
                                                                                                 \
native_type Jam_Call##type##MethodA(JNIEnv *env, jobject obj, jmethodID mID, jvalue *jargs) {    \
    Object *ob = (Object *)obj;                                                                  \
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)mID);                                \
    return mb == NULL ? (native_type)0 :                                                         \
                       *(native_type*)executeMethodList(ob, ob->class, mb, (u8*)jargs);          \
}

#define INT_VIRTUAL_METHOD(type, native_type)                                                    \
native_type Jam_Call##type##Method(JNIEnv *env, jobject obj, jmethodID mID, ...) {               \
    Object *ob = (Object *)obj;                                                                  \
    uintptr_t *ret;                                                                              \
    va_list jargs;                                                                               \
                                                                                                 \
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)mID);                                \
    if(mb == NULL)                                                                               \
        return (native_type)0;                                                                   \
                                                                                                 \
    va_start(jargs, mID);                                                                        \
    ret = executeMethodVaList(ob, ob->class, mb, jargs);                                         \
    va_end(jargs);                                                                               \
                                                                                                 \
    return (native_type)*ret;                                                                    \
}                                                                                                \
                                                                                                 \
native_type Jam_Call##type##MethodV(JNIEnv *env, jobject obj, jmethodID mID, va_list jargs) {    \
    Object *ob = (Object *)obj;                                                                  \
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)mID);                                \
    return mb == NULL ? (native_type)0 :                                                         \
                        (native_type)*(uintptr_t*)executeMethodVaList(ob, ob->class, mb, jargs); \
}                                                                                                \
                                                                                                 \
native_type Jam_Call##type##MethodA(JNIEnv *env, jobject obj, jmethodID mID, jvalue *jargs) {    \
    Object *ob = (Object *)obj;                                                                  \
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)mID);                                \
    return mb == NULL ? (native_type)0 : (native_type)*(uintptr_t*)                              \
                                             executeMethodList(ob, ob->class, mb, (u8*)jargs);   \
}

#define NONVIRTUAL_METHOD(type, native_type)                                                     \
native_type Jam_CallNonvirtual##type##Method(JNIEnv *env, jobject obj, jclass clazz,             \
                jmethodID methodID, ...) {                                                       \
    native_type *ret;                                                                            \
    va_list jargs;                                                                               \
                                                                                                 \
    va_start(jargs, methodID);                                                                   \
    ret = (native_type*)                                                                         \
              executeMethodVaList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, jargs);   \
    va_end(jargs);                                                                               \
                                                                                                 \
    return *ret;                                                                                 \
}                                                                                                \
                                                                                                 \
native_type Jam_CallNonvirtual##type##MethodV(JNIEnv *env, jobject obj, jclass clazz,            \
                jmethodID methodID, va_list jargs) {                                             \
    return *(native_type*)                                                                       \
              executeMethodVaList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, jargs);   \
}                                                                                                \
                                                                                                 \
native_type Jam_CallNonvirtual##type##MethodA(JNIEnv *env, jobject obj, jclass clazz,            \
                jmethodID methodID, jvalue *jargs) {                                             \
    return *(native_type*)                                                                       \
            executeMethodList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs);  \
}

#define INT_NONVIRTUAL_METHOD(type, native_type)                                                 \
native_type Jam_CallNonvirtual##type##Method(JNIEnv *env, jobject obj, jclass clazz,             \
                jmethodID methodID, ...) {                                                       \
    uintptr_t *ret;                                                                              \
    va_list jargs;                                                                               \
                                                                                                 \
    va_start(jargs, methodID);                                                                   \
    ret = executeMethodVaList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, jargs);       \
    va_end(jargs);                                                                               \
                                                                                                 \
    return (native_type)*ret;                                                                    \
}                                                                                                \
                                                                                                 \
native_type Jam_CallNonvirtual##type##MethodV(JNIEnv *env, jobject obj, jclass clazz,            \
                jmethodID methodID, va_list jargs) {                                             \
    return (native_type)*(uintptr_t*)                                                            \
              executeMethodVaList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, jargs);   \
}                                                                                                \
                                                                                                 \
native_type Jam_CallNonvirtual##type##MethodA(JNIEnv *env, jobject obj, jclass clazz,            \
                jmethodID methodID, jvalue *jargs) {                                             \
    return (native_type)*(uintptr_t*)                                                            \
            executeMethodList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs);  \
}

#define STATIC_METHOD(type, native_type)                                                         \
native_type Jam_CallStatic##type##Method(JNIEnv *env, jclass clazz,                              \
                jmethodID methodID, ...) {                                                       \
    native_type *ret;                                                                            \
    va_list jargs;                                                                               \
                                                                                                 \
    va_start(jargs, methodID);                                                                   \
    ret = (native_type*) executeMethodVaList(NULL, (Class*)clazz, (MethodBlock*)methodID, jargs);\
    va_end(jargs);                                                                               \
                                                                                                 \
    return *ret;                                                                                 \
}                                                                                                \
                                                                                                 \
native_type Jam_CallStatic##type##MethodV(JNIEnv *env, jclass clazz, jmethodID methodID,         \
                va_list jargs) {                                                                 \
    return *(native_type*)                                                                       \
            executeMethodVaList(NULL, (Class*)clazz, (MethodBlock*)methodID, jargs);             \
}                                                                                                \
                                                                                                 \
native_type Jam_CallStatic##type##MethodA(JNIEnv *env, jclass clazz, jmethodID methodID,         \
                jvalue *jargs) {                                                                 \
    return *(native_type*)                                                                       \
            executeMethodList(NULL, (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs);          \
}

#define INT_STATIC_METHOD(type, native_type)                                                     \
native_type Jam_CallStatic##type##Method(JNIEnv *env, jclass clazz,                              \
                jmethodID methodID, ...) {                                                       \
    uintptr_t *ret;                                                                              \
    va_list jargs;                                                                               \
                                                                                                 \
    va_start(jargs, methodID);                                                                   \
    ret = executeMethodVaList(NULL, (Class*)clazz, (MethodBlock*)methodID, jargs);               \
    va_end(jargs);                                                                               \
                                                                                                 \
    return (native_type)*ret;                                                                    \
}                                                                                                \
                                                                                                 \
native_type Jam_CallStatic##type##MethodV(JNIEnv *env, jclass clazz, jmethodID methodID,         \
                va_list jargs) {                                                                 \
    return (native_type) *(uintptr_t*)                                                           \
            executeMethodVaList(NULL, (Class*)clazz, (MethodBlock*)methodID, jargs);             \
}                                                                                                \
                                                                                                 \
native_type Jam_CallStatic##type##MethodA(JNIEnv *env, jclass clazz, jmethodID methodID,         \
                jvalue *jargs) {                                                                 \
    return (native_type)*(uintptr_t*)                                                            \
            executeMethodList(NULL, (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs);          \
}

#define CALL_METHOD(access)               \
INT_##access##_METHOD(Boolean, jboolean); \
INT_##access##_METHOD(Byte, jbyte);       \
INT_##access##_METHOD(Char, jchar);       \
INT_##access##_METHOD(Short, jshort);     \
INT_##access##_METHOD(Int, jint);         \
access##_METHOD(Long, jlong);             \
access##_METHOD(Float, jfloat);           \
access##_METHOD(Double, jdouble);

CALL_METHOD(VIRTUAL);
CALL_METHOD(NONVIRTUAL);
CALL_METHOD(STATIC);

jobject Jam_CallObjectMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
    Object *ob = (Object *)obj;
    Object *ret;
    va_list jargs;
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)methodID);
    if(mb == NULL)
        return NULL;

    va_start(jargs, methodID);
    ret = addJNILref(*(Object**) executeMethodVaList(ob, ob->class, mb, jargs));
    va_end(jargs);
    return (jobject)ret;
}

jobject Jam_CallObjectMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list jargs) {
    Object *ob = (Object *)obj;
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)methodID);

    return mb == NULL ? NULL : (jobject)addJNILref(*(Object**)
                                        executeMethodVaList(ob, ob->class, mb, jargs));
}

jobject Jam_CallObjectMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *jargs) {
    Object *ob = (Object *)obj;
    MethodBlock *mb = lookupVirtualMethod(ob, (MethodBlock*)methodID);

    return mb == NULL ? NULL : (jobject)addJNILref(*(Object**)
                                        executeMethodList(ob, ob->class, mb, (u8*)jargs));
}

jobject Jam_CallNonvirtualObjectMethod(JNIEnv *env, jobject obj, jclass clazz,
                jmethodID methodID, ...) {
    Object *ret;
    va_list jargs;
    va_start(jargs, methodID);
    ret = addJNILref(*(Object**) executeMethodVaList((Object*)obj,
                            (Class*)clazz, (MethodBlock*)methodID, jargs));
    va_end(jargs);
    return (jobject)ret;
}

jobject Jam_CallNonvirtualObjectMethodV(JNIEnv *env, jobject obj, jclass clazz,
                jmethodID methodID, va_list jargs) {
    return (jobject)addJNILref(*(Object**) executeMethodVaList((Object*)obj,
                            (Class*)clazz, (MethodBlock*)methodID, jargs));
}

jobject Jam_CallNonvirtualObjectMethodA(JNIEnv *env, jobject obj, jclass clazz,
                jmethodID methodID, jvalue *jargs) {
    return (jobject)addJNILref(*(Object**) executeMethodList((Object*)obj,
                            (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs));
}

jobject Jam_CallStaticObjectMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    Object *ret;
    va_list jargs;
    va_start(jargs, methodID);
    ret = addJNILref(*(Object**) executeMethodVaList(NULL,
                              (Class*)clazz, (MethodBlock*)methodID, jargs));
    va_end(jargs);
    return (jobject)ret;
}

jobject Jam_CallStaticObjectMethodV(JNIEnv *env, jclass clazz,
                jmethodID methodID, va_list jargs) {
    return (jobject)addJNILref(*(Object**) executeMethodVaList(NULL,
                              (Class*)clazz, (MethodBlock*)methodID, jargs));
}

jobject Jam_CallStaticObjectMethodA(JNIEnv *env, jclass clazz,
                jmethodID methodID, jvalue *jargs) {
    return (jobject)addJNILref(*(Object**) executeMethodList(NULL,
                            (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs));
}

void Jam_CallVoidMethod(JNIEnv *env, jobject obj, jmethodID methodID, ...) {
    va_list jargs;
    MethodBlock *mb;
    Object *ob = (Object *)obj;
 
    va_start(jargs, methodID);
    if((mb = lookupVirtualMethod(ob, (MethodBlock*)methodID)) != NULL)
        executeMethodVaList(ob, ob->class, mb, jargs);
    va_end(jargs);
}

void Jam_CallVoidMethodV(JNIEnv *env, jobject obj, jmethodID methodID, va_list jargs) {
    MethodBlock *mb;
    Object *ob = (Object *)obj;
    if((mb = lookupVirtualMethod(ob, (MethodBlock*)methodID)) != NULL)
        executeMethodVaList(ob, ob->class, mb, jargs);
}

void Jam_CallVoidMethodA(JNIEnv *env, jobject obj, jmethodID methodID, jvalue *jargs) {
    MethodBlock *mb;
    Object *ob = (Object *)obj;
    if((mb = lookupVirtualMethod(ob, (MethodBlock*)methodID)) != NULL)
        executeMethodList(ob, ob->class, mb, (u8*)jargs);
}

void Jam_CallNonvirtualVoidMethod(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, ...) {
    va_list jargs;
    va_start(jargs, methodID);
    executeMethodVaList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, jargs);
    va_end(jargs);
}

void Jam_CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz,
                jmethodID methodID, va_list jargs) {
      executeMethodVaList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, jargs);
}

void Jam_CallNonvirtualVoidMethodA(JNIEnv *env, jobject obj, jclass clazz,
                jmethodID methodID, jvalue *jargs) {
    executeMethodList((Object*)obj, (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs);
}

void Jam_CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...) {
    va_list jargs;
    va_start(jargs, methodID);
    executeMethodVaList(NULL, (Class*)clazz, (MethodBlock*)methodID, jargs);
    va_end(jargs);
}

void Jam_CallStaticVoidMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list jargs) {
    executeMethodVaList(NULL, (Class*)clazz, (MethodBlock*)methodID, jargs);
}

void Jam_CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, jvalue *jargs) {
    executeMethodList(NULL, (Class*)clazz, (MethodBlock*)methodID, (u8*)jargs);
}

#define NEW_PRIM_ARRAY(type, native_type, array_type)                             \
native_type##Array Jam_New##type##Array(JNIEnv *env, jsize length) {              \
    return (native_type##Array) addJNILref(allocTypeArray(array_type, length));   \
}

#define GET_PRIM_ARRAY_ELEMENTS(type, native_type)                                              \
native_type *Jam_Get##type##ArrayElements(JNIEnv *env, native_type##Array array,                \
                                          jboolean *isCopy) {                                   \
    if(isCopy != NULL)                                                                          \
        *isCopy = JNI_FALSE;                                                                    \
    addJNIGref((Object*)array);                                                                 \
    return (native_type*)ARRAY_DATA((Object*)array);                                            \
}

#define RELEASE_PRIM_ARRAY_ELEMENTS(type, native_type)                                          \
void Jam_Release##type##ArrayElements(JNIEnv *env, native_type##Array array,                    \
                                      native_type *elems, jint mode) {                          \
    delJNIGref((Object*)array);                                                                 \
}

#define GET_PRIM_ARRAY_REGION(type, native_type)                                                \
void Jam_Get##type##ArrayRegion(JNIEnv *env, native_type##Array array, jsize start,             \
                                jsize len, native_type *buf) {                                  \
    memcpy(buf, (native_type*)ARRAY_DATA((Object*)array) + start, len * sizeof(native_type));   \
}

#define SET_PRIM_ARRAY_REGION(type, native_type)                                                \
void Jam_Set##type##ArrayRegion(JNIEnv *env, native_type##Array array, jsize start, jsize len,  \
                                native_type *buf) {                                             \
    memcpy((native_type*)ARRAY_DATA((Object*)array) + start, buf, len * sizeof(native_type));   \
}

#define PRIM_ARRAY_OP(type, native_type, array_type) \
    NEW_PRIM_ARRAY(type, native_type, array_type);   \
    GET_PRIM_ARRAY_ELEMENTS(type, native_type);      \
    RELEASE_PRIM_ARRAY_ELEMENTS(type, native_type);  \
    GET_PRIM_ARRAY_REGION(type, native_type);        \
    SET_PRIM_ARRAY_REGION(type, native_type);

PRIM_ARRAY_OP(Boolean, jboolean, T_BOOLEAN);
PRIM_ARRAY_OP(Byte, jbyte, T_BYTE);
PRIM_ARRAY_OP(Char, jchar, T_CHAR);
PRIM_ARRAY_OP(Short, jshort, T_SHORT);
PRIM_ARRAY_OP(Int, jint, T_INT);
PRIM_ARRAY_OP(Long, jlong, T_LONG);
PRIM_ARRAY_OP(Float, jfloat, T_FLOAT);
PRIM_ARRAY_OP(Double, jdouble, T_DOUBLE);

#define METHOD(type, ret_type)                \
    Jam_Call##type##ret_type##Method,         \
    Jam_Call##type##ret_type##MethodV,        \
    Jam_Call##type##ret_type##MethodA

#define METHODS(type)                         \
    METHOD(type, Object),                     \
    METHOD(type, Boolean),                    \
    METHOD(type, Byte),                       \
    METHOD(type, Char),                       \
    METHOD(type, Short),                      \
    METHOD(type, Int),                        \
    METHOD(type, Long),                       \
    METHOD(type, Float),                      \
    METHOD(type, Double),                     \
    METHOD(type, Void)

#define FIELD(direction, type, field_type)    \
    Jam_##direction##type##field_type##Field

#define FIELDS2(direction, type)              \
        FIELD(direction, type,  Object),      \
        FIELD(direction, type, Boolean),      \
        FIELD(direction, type,  Byte),        \
        FIELD(direction, type, Char),         \
        FIELD(direction, type, Short),        \
        FIELD(direction, type, Int),          \
        FIELD(direction, type, Long),         \
        FIELD(direction, type, Float),        \
        FIELD(direction, type, Double)

#define FIELDS(type)                          \
        FIELDS2(Get, type),                   \
        FIELDS2(Set, type)

#define ARRAY(op, el_type, type)              \
        Jam_##op##el_type##Array##type

#define ARRAY_OPS(op, type)                   \
        ARRAY(op, Boolean, type),             \
        ARRAY(op, Byte, type),                \
        ARRAY(op, Char, type),                \
        ARRAY(op, Short, type),               \
        ARRAY(op, Int, type),                 \
        ARRAY(op, Long, type),                \
        ARRAY(op, Float, type),               \
        ARRAY(op, Double, type)

struct _JNINativeInterface Jam_JNINativeInterface = {
    NULL,
    NULL,
    NULL,
    NULL,
    Jam_GetVersion,
    Jam_DefineClass,
    Jam_FindClass,
    Jam_FromReflectedMethod,
    Jam_FromReflectedField,
    Jam_ToReflectedMethod,
    Jam_GetSuperClass,
    Jam_IsAssignableFrom,
    Jam_ToReflectedField,
    Jam_Throw,
    Jam_ThrowNew,
    Jam_ExceptionOccurred,
    Jam_ExceptionDescribe,
    Jam_ExceptionClear,
    Jam_FatalError,
    Jam_PushLocalFrame,
    Jam_PopLocalFrame,
    Jam_NewGlobalRef,
    Jam_DeleteGlobalRef,
    Jam_DeleteLocalRef,
    Jam_IsSameObject,
    Jam_NewLocalRef,
    Jam_EnsureLocalCapacity,
    Jam_AllocObject,
    Jam_NewObject,
    Jam_NewObjectV,
    Jam_NewObjectA,
    Jam_GetObjectClass,
    Jam_IsInstanceOf,
    Jam_GetMethodID,
    METHODS(/*virtual*/),
    METHODS(Nonvirtual),
    Jam_GetFieldID,
    FIELDS(/*instance*/),
    Jam_GetStaticMethodID,
    METHODS(Static),
    Jam_GetStaticFieldID,
    FIELDS(Static),
    Jam_NewString,
    Jam_GetStringLength,
    Jam_GetStringChars,
    Jam_ReleaseStringChars,
    Jam_NewStringUTF,
    Jam_GetStringUTFLength,
    Jam_GetStringUTFChars,
    Jam_ReleaseStringUTFChars,
    Jam_GetArrayLength,
    ARRAY(New, Object,),
    ARRAY(Get, Object, Element),
    ARRAY(Set, Object, Element),
    ARRAY_OPS(New,),
    ARRAY_OPS(Get, Elements),
    ARRAY_OPS(Release, Elements),
    ARRAY_OPS(Get, Region),
    ARRAY_OPS(Set, Region),
    Jam_RegisterNatives,
    Jam_UnregisterNatives,
    Jam_MonitorEnter,
    Jam_MonitorExit,
    Jam_GetJavaVM,
    Jam_GetStringRegion,
    Jam_GetStringUTFRegion,
    Jam_GetPrimitiveArrayCritical,
    Jam_ReleasePrimitiveArrayCritical,
    Jam_GetStringCritical,
    Jam_ReleaseStringCritical,
    Jam_NewWeakGlobalRef,
    Jam_DeleteWeakGlobalRef,
    Jam_ExceptionCheck,
    Jam_NewDirectByteBuffer,
    Jam_GetDirectBufferAddress,
    Jam_GetDirectBufferCapacity
};

jint Jam_DestroyJavaVM(JavaVM *vm) {
    mainThreadWaitToExitVM();
    exitVM(0);

    return JNI_OK;
}

static void *env = &Jam_JNINativeInterface;

static jint attachCurrentThread(void **penv, void *args, int is_daemon) {
    if(threadSelf() == NULL) {
        char *name = NULL;
        Object *group = NULL;

        if(args != NULL) {
            JavaVMAttachArgs *attach_args = (JavaVMAttachArgs*)args;
            if((attach_args->version != JNI_VERSION_1_4) && (attach_args->version != JNI_VERSION_1_2))
                return JNI_EVERSION;

            name = attach_args->name;
            group = attach_args->group;
        }

        if(attachJNIThread(name, is_daemon, group) == NULL)
            return JNI_ERR;

        initJNILrefs();
    }

    *penv = &env;
    return JNI_OK;
}

jint Jam_AttachCurrentThread(JavaVM *vm, void **penv, void *args) {
    return attachCurrentThread(penv, args, FALSE);
}

jint Jam_AttachCurrentThreadAsDaemon(JavaVM *vm, void **penv, void *args) {
    return attachCurrentThread(penv, args, TRUE);
}

jint Jam_DetachCurrentThread(JavaVM *vm) {
    Thread *thread = threadSelf();

    if(thread == NULL)
        return JNI_EDETACHED;

    detachJNIThread(thread);
    return JNI_OK;
}

jint Jam_GetEnv(JavaVM *vm, void **penv, jint version) {
    if((version != JNI_VERSION_1_4) && (version != JNI_VERSION_1_2)
                                    && (version != JNI_VERSION_1_1)) {
        *penv = NULL;
        return JNI_EVERSION;
    }

    if(threadSelf() == NULL) {
        *penv = NULL;
        return JNI_EDETACHED;
    }

    *penv = &env;
    return JNI_OK;
}

struct _JNIInvokeInterface Jam_JNIInvokeInterface = {
    NULL,
    NULL,
    NULL,
    Jam_DestroyJavaVM,
    Jam_AttachCurrentThread,
    Jam_DetachCurrentThread,
    Jam_GetEnv,
    Jam_AttachCurrentThreadAsDaemon,
};

jint JNI_GetDefaultJavaVMInitArgs(void *args) {
    JavaVMInitArgs *vm_args = (JavaVMInitArgs*) args;

    if((vm_args->version != JNI_VERSION_1_4) && (vm_args->version != JNI_VERSION_1_2))
        return JNI_EVERSION;

    return JNI_OK;
}

jint parseInitOptions(JavaVMInitArgs *vm_args, InitArgs *args) {
    Property props[vm_args->nOptions];
    int props_count = 0;
    int i;

    for(i = 0; i < vm_args->nOptions; i++) {
        char *string = vm_args->options[i].optionString;

        if(strcmp(string, "vfprintf") == 0)
            args->vfprintf = vm_args->options[i].extraInfo;

        else if(strcmp(string, "exit") == 0)
            args->exit = vm_args->options[i].extraInfo;

        else if(strcmp(string, "abort") == 0)
            args->abort = vm_args->options[i].extraInfo;

        else if(strncmp(string, "-verbose:", 9) == 0) {
            char *type = &string[8];

            do {
                type++;

                if(strncmp(type, "class", 5) == 0) {
                    args->verboseclass = TRUE;
                    type += 5;
                 }
                else if(strncmp(type, "gc", 2) == 0) {
                    args->verbosegc = TRUE;
                    type += 2;
                }
                else if(strncmp(type, "jni", 3) == 0) {
                    args->verbosedll = TRUE;
                    type += 3;
                }
            } while(*type == ',');

        } else if(strcmp(string, "-Xnoasyncgc") == 0)
            args->noasyncgc = TRUE;

        else if(strncmp(string, "-Xms", 4) == 0) {
            args->min_heap = parseMemValue(string + 4);
            if(args->min_heap < MIN_HEAP)
                goto error;

        } else if(strncmp(string, "-Xmx", 4) == 0) {
            args->max_heap = parseMemValue(string + 4);
            if(args->max_heap < MIN_HEAP)
                goto error;

        } else if(strncmp(string, "-Xss", 4) == 0) {
            args->java_stack = parseMemValue(string + 4);
            if(args->java_stack < MIN_STACK)
                goto error;

        } else if(strncmp(string, "-D", 2) == 0) {
            char *pntr, *key = strcpy(sysMalloc(strlen(string+2) + 1), string+2);
            for(pntr = key; *pntr && (*pntr != '='); pntr++);
            if(pntr == key)
                goto error;

            *pntr++ = '\0';
            props[props_count].key = key;
            props[props_count++].value = pntr;

        } else if(strncmp(string, "-Xbootclasspath:", 16) == 0) {

            args->bootpathopt = '\0';
            args->bootpath = string + 16;

        } else if(strncmp(string, "-Xbootclasspath/a:", 18) == 0 ||
                  strncmp(string, "-Xbootclasspath/p:", 18) == 0 ||
                  strncmp(string, "-Xbootclasspath/c:", 18) == 0 ||
                  strncmp(string, "-Xbootclasspath/v:", 18) == 0) {

            args->bootpathopt = string[16];
            args->bootpath = string + 18;

        } else if(strcmp(string, "-Xnocompact") == 0) {
            args->compact_specified = TRUE;
            args->do_compact = FALSE;

        } else if(strcmp(string, "-Xcompactalways") == 0) {
            args->compact_specified = args->do_compact = TRUE;

        } else if(!vm_args->ignoreUnrecognized)
            goto error;
    }

    if(args->min_heap > args->max_heap)
        goto error;

    if((args->props_count = props_count)) {
        args->commandline_props = (Property*)sysMalloc(props_count * sizeof(Property));
        memcpy(args->commandline_props, &props[0], props_count * sizeof(Property));
    }

    return JNI_OK;

error:
    return JNI_ERR;
}

jint JNI_CreateJavaVM(JavaVM **pvm, void **penv, void *args) {
    JavaVMInitArgs *vm_args = (JavaVMInitArgs*) args;
    InitArgs init_args;

    if((vm_args->version != JNI_VERSION_1_4) && (vm_args->version != JNI_VERSION_1_2))
        return JNI_EVERSION;

    setDefaultInitArgs(&init_args);

    if(parseInitOptions(vm_args, &init_args) == JNI_ERR)
        return JNI_ERR;

    init_args.main_stack_base = nativeStackBase();
    initVM(&init_args);
    initJNILrefs();

    *penv = &env;
    *pvm = &invokeIntf;

    return JNI_OK;
}

jint JNI_GetCreatedJavaVMs(JavaVM **buff, jsize buff_len, jsize *num) {
    if(buff_len > 0) {
        *buff = &invokeIntf;
        *num = 1;
        return JNI_OK;
    }
    return JNI_ERR;
}

void *getJNIInterface() {
    return &Jam_JNINativeInterface;
}
#endif
