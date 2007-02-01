/*
 * Copyright (C) 2003, 2004, 2005, 2006 Robert Lougher <rob@lougher.org.uk>.
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

#ifdef NO_JNI
#error to use classpath, Jam must be compiled with JNI!
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "jam.h"
#include "alloc.h"
#include "thread.h"
#include "lock.h"
#include "natives.h"

static int pd_offset;

void initialiseNatives() {
    FieldBlock *pd = findField(java_lang_Class, "pd", "Ljava/security/ProtectionDomain;");

    if(pd == NULL) {
        jam_fprintf(stderr, "Error initialising VM (initialiseNatives)\n");
        exitVM(1);
    }
    pd_offset = pd->offset;
}

/* java.lang.VMObject */

uintptr_t *getClass(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *ob = (Object*)*ostack;
    *ostack++ = (uintptr_t)ob->class;
    return ostack;
}

uintptr_t *jamClone(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *ob = (Object*)*ostack;
    *ostack++ = (uintptr_t)cloneObject(ob);
    return ostack;
}

/* static method wait(Ljava/lang/Object;JI)V */
uintptr_t *jamWait(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *obj = (Object *)ostack[0];
    long long ms = *((long long *)&ostack[1]);
    int ns = ostack[3];

    objectWait(obj, ms, ns);
    return ostack;
}

/* static method notify(Ljava/lang/Object;)V */
uintptr_t *notify(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *obj = (Object *)*ostack;
    objectNotify(obj);
    return ostack;
}

/* static method notifyAll(Ljava/lang/Object;)V */
uintptr_t *notifyAll(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *obj = (Object *)*ostack;
    objectNotifyAll(obj);
    return ostack;
}

/* java.lang.VMSystem */

/* arraycopy(Ljava/lang/Object;ILjava/lang/Object;II)V */
uintptr_t *arraycopy(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *src = (Object *)ostack[0];
    int start1 = ostack[1];
    Object *dest = (Object *)ostack[2];
    unsigned long start2 = ostack[3];
    unsigned long length = ostack[4];

    if((src == NULL) || (dest == NULL))
        signalException("java/lang/NullPointerException", NULL);
    else {
        ClassBlock *scb = CLASS_CB(src->class);
        ClassBlock *dcb = CLASS_CB(dest->class);
        char *sdata = ARRAY_DATA(src);            
        char *ddata = ARRAY_DATA(dest);            

        if((scb->name[0] != '[') || (dcb->name[0] != '['))
            goto storeExcep; 

        if((start1 < 0) || (start2 < 0) || (length < 0)
                        || ((start1 + length) > ARRAY_LEN(src))
                        || ((start2 + length) > ARRAY_LEN(dest))) {
            signalException("java/lang/ArrayIndexOutOfBoundsException", NULL);
            return ostack;
        }

        if(isInstanceOf(dest->class, src->class)) {
            int size;

            switch(scb->name[1]) {
                case 'B':
                case 'Z':
                    size = 1;
                    break;
                case 'C':
                case 'S':
                    size = 2;
                    break;
                case 'I':
                case 'F':
                    size = 4;
                    break;
                case 'L':
                case '[':
                    size = sizeof(Object*);
                    break;
                case 'J':
                case 'D':
		default:
                    size = 8;
                    break;
            } 

            memmove(ddata + start2*size, sdata + start1*size, length*size);
        } else {
            Object **sob, **dob;
            unsigned long i;

            if(!(((scb->name[1] == 'L') || (scb->name[1] == '[')) &&
                          ((dcb->name[1] == 'L') || (dcb->name[1] == '['))))
                goto storeExcep; 

            /* Not compatible array types, but elements may be compatible...
               e.g. src = [Ljava/lang/Object, dest = [Ljava/lang/String, but
               all src = Strings - check one by one...
             */
            
            if(scb->dim > dcb->dim)
                goto storeExcep;

            sob = &((Object**)sdata)[start1];
            dob = &((Object**)ddata)[start2];

            for(i = 0; i < length; i++) {
                if((*sob != NULL) && !arrayStoreCheck(dest->class, (*sob)->class))
                    goto storeExcep;
                *dob++ = *sob++;
            }
        }
    }
    return ostack;

storeExcep:
    signalException("java/lang/ArrayStoreException", NULL);
    return ostack;
}

uintptr_t *identityHashCode(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *ob = (Object*)*ostack;
    uintptr_t addr = ob == NULL ? 0 : getObjectHashcode(ob);

    *ostack++ = addr & 0xffffffff;
    return ostack;
}

/* java.lang.VMRuntime */

uintptr_t *availableProcessors(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = nativeAvailableProcessors();
    return ostack;
}

uintptr_t *freeMemory(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *(u8*)ostack = freeHeapMem();
    return ostack + 2;
}

uintptr_t *totalMemory(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *(u8*)ostack = totalHeapMem();
    return ostack + 2;
}

uintptr_t *maxMemory(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *(u8*)ostack = maxHeapMem();
    return ostack + 2;
}

uintptr_t *gc(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    gc1();
    return ostack;
}

uintptr_t *runFinalization(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    runFinalizers();
    return ostack;
}

uintptr_t *exitInternal(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    int status = ostack[0];
    jamvm_exit(status);
}

uintptr_t *nativeLoad(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    char *name = String2Cstr((Object*)ostack[0]);
    Object *class_loader = (Object *)ostack[1];

    ostack[0] = resolveDll(name, class_loader);
    free(name);

    return ostack+1;
}

uintptr_t *mapLibraryName(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    char *name = String2Cstr((Object*)ostack[0]);
    char *lib = getDllName(name);
    free(name);

    *ostack++ = (uintptr_t)Cstr2String(lib);
    free(lib);

    return ostack;
}

uintptr_t *propertiesPreInit(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *properties = (Object *)*ostack;
    addDefaultProperties(properties);
    return ostack;
}

uintptr_t *propertiesPostInit(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *properties = (Object *)*ostack;
    addCommandLineProperties(properties);
    return ostack;
}

/* java.lang.VMClass */

#define GET_CLASS(vmClass) (Class*)vmClass

uintptr_t *isInstance(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    Object *ob = (Object*)ostack[1];

    *ostack++ = ob == NULL ? FALSE : (uintptr_t)isInstanceOf(clazz, ob->class);
    return ostack;
}

uintptr_t *isAssignableFrom(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    Class *clazz2 = (Class*)ostack[1];

    if(clazz2 == NULL)
        signalException("java/lang/NullPointerException", NULL);
    else
        *ostack++ = (uintptr_t)isInstanceOf(clazz, clazz2);

    return ostack;
}

uintptr_t *isInterface(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = IS_INTERFACE(cb) ? TRUE : FALSE;
    return ostack;
}

uintptr_t *isPrimitive(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = IS_PRIMITIVE(cb) ? TRUE : FALSE;
    return ostack;
}

uintptr_t *isArray(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = IS_ARRAY(cb) ? TRUE : FALSE;
    return ostack;
}

uintptr_t *isMember(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = IS_MEMBER(cb) ? TRUE : FALSE;
    return ostack;
}

uintptr_t *isLocal(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = IS_LOCAL(cb) ? TRUE : FALSE;
    return ostack;
}

uintptr_t *isAnonymous(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = IS_ANONYMOUS(cb) ? TRUE : FALSE;
    return ostack;
}

uintptr_t *getEnclosingClass0(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    *ostack++ = (uintptr_t) getEnclosingClass(clazz);
    return ostack;
}

uintptr_t *getEnclosingMethod0(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    *ostack++ = (uintptr_t) getEnclosingMethodObject(clazz);
    return ostack;
}

uintptr_t *getEnclosingConstructor(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    *ostack++ = (uintptr_t) getEnclosingConstructorObject(clazz);
    return ostack;
}

uintptr_t *getClassSignature(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    Object *string = NULL;

    if(cb->signature != NULL) {
        char *dot_name = slash2dots(cb->signature);
        string = createString(dot_name);
        free(dot_name);
    }

    *ostack++ = (uintptr_t)string;
    return ostack;
}

uintptr_t *getSuperclass(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    ClassBlock *cb = CLASS_CB(GET_CLASS(ostack[0]));
    *ostack++ = (uintptr_t) (IS_PRIMITIVE(cb) || IS_INTERFACE(cb) ? NULL : cb->super);
    return ostack;
}

uintptr_t *getComponentType(Class *clazz, MethodBlock *mb, uintptr_t *ostack) {
    Class *class = GET_CLASS(ostack[0]);
    ClassBlock *cb = CLASS_CB(class);
    Class *componentType = NULL;

    if(IS_ARRAY(cb))
        switch(cb->name[1]) {
            case '[':
                componentType = findArrayClassFromClass(&cb->name[1], class);
                break;

            default:
                componentType = cb->element_class;
                break;
        }
 
    *ostack++ = (uintptr_t) componentType;
    return ostack;
}

uintptr_t *getName(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    char *dot_name = slash2dots(CLASS_CB((GET_CLASS(*ostack)))->name);
    Object *string = createString(dot_name);
    *ostack++ = (uintptr_t)string;
    free(dot_name);
    return ostack;
}

uintptr_t *getDeclaredClasses(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    int public = ostack[1];
    *ostack++ = (uintptr_t) getClassClasses(clazz, public);
    return ostack;
}

uintptr_t *getDeclaringClass0(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    *ostack++ = (uintptr_t) getDeclaringClass(clazz);
    return ostack;
}

uintptr_t *getDeclaredConstructors(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    int public = ostack[1];
    *ostack++ = (uintptr_t) getClassConstructors(clazz, public);
    return ostack;
}

uintptr_t *getDeclaredMethods(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    int public = ostack[1];
    *ostack++ = (uintptr_t) getClassMethods(clazz, public);
    return ostack;
}

uintptr_t *getDeclaredFields(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    int public = ostack[1];
    *ostack++ = (uintptr_t) getClassFields(clazz, public);
    return ostack;
}

uintptr_t *getInterfaces(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    *ostack++ = (uintptr_t) getClassInterfaces(clazz);
    return ostack;
}

uintptr_t *getClassLoader(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(*ostack);
    *ostack++ = (uintptr_t)CLASS_CB(clazz)->class_loader;
    return ostack;
}

uintptr_t *getClassModifiers(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = GET_CLASS(ostack[0]);
    int ignore_inner_attrs = ostack[1];
    ClassBlock *cb = CLASS_CB(clazz);

    if(!ignore_inner_attrs && cb->declaring_class)
        *ostack++ = (uintptr_t)cb->inner_access_flags;
    else
        *ostack++ = (uintptr_t)cb->access_flags;

    return ostack;
}

uintptr_t *forName0(uintptr_t *ostack, int resolve, Object *loader) {
    Object *string = (Object *)ostack[0];
    Class *class = NULL;
    int len, i = 0;
    char *cstr;
    
    if(string == NULL) {
        signalException("java/lang/NullPointerException", NULL);
        return ostack;
    }

    cstr = String2Utf8(string);
    len = strlen(cstr);

    /* Check the classname to see if it's valid.  It can be
       a 'normal' class or an array class, starting with a [ */

    if(cstr[0] == '[') {
        for(; cstr[i] == '['; i++);
        switch(cstr[i]) {
            case 'Z':
            case 'B':
            case 'C':
            case 'S':
            case 'I':
            case 'F':
            case 'J':
            case 'D':
                if(len-i != 1)
                    goto out;
                break;
            case 'L':
                if(cstr[i+1] == '[' || cstr[len-1] != ';')
                    goto out;
                break;
            default:
                goto out;
                break;
        }
    }

    /* Scan the classname and convert it to internal form
       by converting dots to slashes.  Reject classnames
       containing slashes, as this is an invalid character */

    for(; i < len; i++) {
        if(cstr[i] == '/')
            goto out;
        if(cstr[i] == '.')
            cstr[i] = '/';
    }

    class = findClassFromClassLoader(cstr, loader);

out:
    if(class == NULL) {
        Object *e = exceptionOccured();
        clearException();
        signalChainedException("java/lang/ClassNotFoundException", cstr, e);
    } else
        if(resolve)
            initClass(class);

    free(cstr);
    *ostack++ = (uintptr_t)class;
    return ostack;
}

uintptr_t *forName(Class *clazz, MethodBlock *mb, uintptr_t *ostack) {
    int init = ostack[1];
    Object *loader = (Object*)ostack[2];
    return forName0(ostack, init, loader);
}

uintptr_t *throwException(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *excep = (Object *)ostack[0];
    setException(excep);
    return ostack;
}

uintptr_t *hasClassInitializer(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = (Class*)ostack[0];
    *ostack++ = findMethod(clazz, "<clinit>", "()V") == NULL ? FALSE : TRUE;
    return ostack;
}

/* java.lang.VMThrowable */

uintptr_t *fillInStackTrace(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = (uintptr_t) setStackTrace();
    return ostack;
}

uintptr_t *getStackTrace(Class *class, MethodBlock *m, uintptr_t *ostack) {
    Object *this = (Object *)*ostack;
    *ostack++ = (uintptr_t) convertStackTrace(this);
    return ostack;
}

/* gnu.classpath.VMStackWalker */

uintptr_t *getCallingClass(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = (uintptr_t) getCallerCallerClass();
    return ostack;
}

uintptr_t *getCallingClassLoader(Class *clazz, MethodBlock *mb, uintptr_t *ostack) {
    Class *class = getCallerCallerClass();

    *ostack++ = (uintptr_t) (class ? CLASS_CB(class)->class_loader : NULL);
    return ostack;
}

uintptr_t *getClassContext(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *class_class = findArrayClass("[Ljava/lang/Class;");
    Object *array;
    Frame *last;

    if(class_class == NULL)
        return ostack;

    if((last = getCallerFrame(getExecEnv()->last_frame)) == NULL)
        array = allocArray(class_class, 0, sizeof(Class*)); 
    else {
        Frame *bottom = last;
        int depth = 0;

        do {
            for(; last->mb != NULL; last = last->prev, depth++);
        } while((last = last->prev)->prev != NULL);
    
        array = allocArray(class_class, depth, sizeof(Class*));

        if(array != NULL) {
            Class **data = ARRAY_DATA(array);

            do {
                for(; bottom->mb != NULL; bottom = bottom->prev)
                    *data++ = bottom->mb->class;
            } while((bottom = bottom->prev)->prev != NULL);
        }
    }

    *ostack++ = (uintptr_t)array;
    return ostack;
}

uintptr_t *firstNonNullClassLoader(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Frame *last = getExecEnv()->last_frame;
    Object *loader = NULL;

    do {
        for(; last->mb != NULL; last = last->prev)
            if((loader = CLASS_CB(last->mb->class)->class_loader) != NULL)
                goto out;
    } while((last = last->prev)->prev != NULL);

out:
    *ostack++ = (uintptr_t)loader;
    return ostack;
}

/* java.lang.VMClassLoader */

/* loadClass(Ljava/lang/String;I)Ljava/lang/Class; */
uintptr_t *loadClass(Class *clazz, MethodBlock *mb, uintptr_t *ostack) {
    int resolve = ostack[1];
    return forName0(ostack, resolve, NULL);
}

/* getPrimitiveClass(C)Ljava/lang/Class; */
uintptr_t *getPrimitiveClass(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    char prim_type = *ostack;
    *ostack++ = (uintptr_t)findPrimitiveClass(prim_type);
    return ostack;
}

uintptr_t *defineClass0(Class *clazz, MethodBlock *mb, uintptr_t *ostack) {
    Object *class_loader = (Object *)ostack[0];
    Object *string = (Object *)ostack[1];
    Object *array = (Object *)ostack[2];
    unsigned long offset = ostack[3];
    unsigned long data_len = ostack[4];
    uintptr_t pd = ostack[5];
    Class *class = NULL;

    if(array == NULL)
        signalException("java/lang/NullPointerException", NULL);
    else
        if((offset < 0) || (data_len < 0) ||
                           ((offset + data_len) > ARRAY_LEN(array)))
            signalException("java/lang/ArrayIndexOutOfBoundsException", NULL);
        else {
            char *data = ARRAY_DATA(array);
            char *cstr = string ? String2Utf8(string) : NULL;
            int len = string ? strlen(cstr) : 0;
            int i;

            for(i = 0; i < len; i++)
                if(cstr[i]=='.') cstr[i]='/';

            if((class = defineClass(cstr, data, offset, data_len, class_loader)) != NULL) {
                INST_DATA(class)[pd_offset] = pd;
                linkClass(class);
            }

            free(cstr);
        }

    *ostack++ = (uintptr_t) class;
    return ostack;
}

uintptr_t *findLoadedClass(Class *clazz, MethodBlock *mb, uintptr_t *ostack) {
    Object *class_loader = (Object *)ostack[0];
    Object *string = (Object *)ostack[1];
    Class *class;
    char *cstr;
    int len, i;

    if(string == NULL) {
        signalException("java/lang/NullPointerException", NULL);
        return ostack;
    }

    cstr = String2Cstr(string);
    len = strlen(cstr);

    for(i = 0; i < len; i++)
        if(cstr[i]=='.') cstr[i]='/';

    class = findHashedClass(cstr, class_loader);

    free(cstr);
    *ostack++ = (uintptr_t) class;
    return ostack;
}

uintptr_t *resolveClass0(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *clazz = (Class *)*ostack;
    initClass(clazz);
    return ostack;
}

uintptr_t *getBootClassPathSize(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = bootClassPathSize();
    return ostack;
}

uintptr_t *getBootClassPathResource(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *string = (Object *) ostack[0];
    char *filename = String2Cstr(string);
    int index = ostack[1];

    *ostack++ = (uintptr_t) bootClassPathResource(filename, index);
    return ostack;
}

/* java.lang.reflect.Constructor */

uintptr_t *constructNative(Class *class, MethodBlock *mb2, uintptr_t *ostack) {
    Object *array = (Object*)ostack[1]; 
    Class *decl_class = (Class*)ostack[2];
    Object *param_types = (Object*)ostack[3];
    ClassBlock *cb = CLASS_CB(decl_class); 
    MethodBlock *mb = &(cb->methods[ostack[4]]); 
    int no_access_check = ostack[5]; 
    Object *ob = NULL;

    if(cb->access_flags & ACC_ABSTRACT)
        signalException("java/lang/InstantiationError", cb->name);
    else
        ob = allocObject(mb->class);

    if(ob) invoke(ob, mb, array, param_types, !no_access_check);

    *ostack++ = (uintptr_t) ob;
    return ostack;
}

uintptr_t *getMethodModifiers(Class *class, MethodBlock *mb2, uintptr_t *ostack) {
    Class *decl_class = (Class*)ostack[1];
    MethodBlock *mb = &(CLASS_CB(decl_class)->methods[ostack[2]]); 
    *ostack++ = (uintptr_t) mb->access_flags;
    return ostack;
}

uintptr_t *getMethodSignature(Class *class, MethodBlock *mb2, uintptr_t *ostack) {
    Class *decl_class = (Class*)ostack[1];
    MethodBlock *mb = &(CLASS_CB(decl_class)->methods[ostack[2]]); 
    Object *string = NULL;

    if(mb->signature != NULL) {
        char *dot_name = slash2dots(mb->signature);
        string = createString(dot_name);
        free(dot_name);
    }

    *ostack++ = (uintptr_t)string;
    return ostack;
}

uintptr_t *getFieldModifiers(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *decl_class = (Class*)ostack[1];
    FieldBlock *fb = &(CLASS_CB(decl_class)->fields[ostack[2]]); 
    *ostack++ = (uintptr_t) fb->access_flags;
    return ostack;
}

uintptr_t *getFieldSignature(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *decl_class = (Class*)ostack[1];
    FieldBlock *fb = &(CLASS_CB(decl_class)->fields[ostack[2]]); 
    Object *string = NULL;

    if(fb->signature != NULL) {
        char *dot_name = slash2dots(fb->signature);
        string = createString(dot_name);
        free(dot_name);
    }

    *ostack++ = (uintptr_t)string;
    return ostack;
}

Object *getAndCheckObject(uintptr_t *ostack, Class *type) {
    Object *ob = (Object*)ostack[1];

    if(ob == NULL) {
        signalException("java/lang/NullPointerException", NULL);
        return NULL;
    }

    if(!isInstanceOf(type, ob->class)) {
        signalException("java/lang/IllegalArgumentException",
                        "object is not an instance of declaring class");
        return NULL;
    }

    return ob;
}

uintptr_t *getPntr2Field(uintptr_t *ostack) {
    Class *decl_class = (Class *)ostack[2];
    FieldBlock *fb = &(CLASS_CB(decl_class)->fields[ostack[4]]); 
    int no_access_check = ostack[5];
    Object *ob;

    if(!no_access_check) {
        Class *caller = getCallerCallerClass();
        if(!checkClassAccess(decl_class, caller) || !checkFieldAccess(fb, caller)) {
            signalException("java/lang/IllegalAccessException", "field is not accessible");
            return NULL;
        }
    }

    if(fb->access_flags & ACC_STATIC) {
        initClass(decl_class);
        return &fb->static_value;
    }

    if((ob = getAndCheckObject(ostack, decl_class)) == NULL)
        return NULL;

    return &(INST_DATA(ob)[fb->offset]);
}

uintptr_t *getField(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *field_type = (Class *)ostack[3];
    uintptr_t *field;

    /* If field is static, getPntr2Field also initialises the field's declaring class */
    if((field = getPntr2Field(ostack)) != NULL)
        *ostack++ = (uintptr_t) createWrapperObject(field_type, field);

    return ostack;
}

uintptr_t *getPrimitiveField(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *field_type = (Class *)ostack[3];
    int type_no = ostack[6]; 

    ClassBlock *type_cb = CLASS_CB(field_type);
    uintptr_t *field;

    /* If field is static, getPntr2Field also initialises the field's declaring class */
    if(((field = getPntr2Field(ostack)) != NULL) && (!(IS_PRIMITIVE(type_cb)) ||
                 ((ostack = widenPrimitiveValue(getPrimTypeIndex(type_cb), type_no, field, ostack)) == NULL)))
        signalException("java/lang/IllegalArgumentException", "field type mismatch");

    return ostack;
}

uintptr_t *setField(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *field_type = (Class *)ostack[3];
    Object *value = (Object*)ostack[6];
    uintptr_t *field;

    /* If field is static, getPntr2Field also initialises the field's declaring class */
    if(((field = getPntr2Field(ostack)) != NULL) &&
                     (unwrapAndWidenObject(field_type, value, field) == NULL))
        signalException("java/lang/IllegalArgumentException", "field type mismatch");

    return ostack;
}

uintptr_t *setPrimitiveField(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *field_type = (Class *)ostack[3];
    int type_no = ostack[6]; 

    ClassBlock *type_cb = CLASS_CB(field_type);
    uintptr_t *field;

    /* If field is static, getPntr2Field also initialises the field's declaring class */
    if(((field = getPntr2Field(ostack)) != NULL) && (!(IS_PRIMITIVE(type_cb)) ||
                 (widenPrimitiveValue(type_no, getPrimTypeIndex(type_cb), &ostack[7], field) == NULL)))
        signalException("java/lang/IllegalArgumentException", "field type mismatch");

    return ostack;
}

/* java.lang.reflect.Method */

uintptr_t *invokeNative(Class *class, MethodBlock *mb2, uintptr_t *ostack) {
    Object *array = (Object*)ostack[2]; 
    Class *decl_class = (Class*)ostack[3];
    Object *param_types = (Object*)ostack[4];
    Class *ret_type = (Class*)ostack[5];
    MethodBlock *mb = &(CLASS_CB(decl_class)->methods[ostack[6]]); 
    int no_access_check = ostack[7]; 
    Object *ob = NULL;
    uintptr_t *ret;

    /* If it's a static method, class may not be initialised */
    if(mb->access_flags & ACC_STATIC)
        initClass(decl_class);
    else {
        /* Interfaces are not normally initialsed. */
        if(IS_INTERFACE(CLASS_CB(decl_class)))
            initClass(decl_class);

        if(((ob = getAndCheckObject(ostack, decl_class)) == NULL) ||
                                     ((mb = lookupVirtualMethod(ob, mb)) == NULL))
            return ostack;
    }
 
    if((ret = (uintptr_t*) invoke(ob, mb, array, param_types, !no_access_check)) != NULL)
        *ostack++ = (uintptr_t) createWrapperObject(ret_type, ret);

    return ostack;
}

/* java.lang.VMString */

/* static method - intern(Ljava/lang/String;)Ljava/lang/String; */
uintptr_t *intern(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *string = (Object*)ostack[0];
    ostack[0] = (uintptr_t)findInternedString(string);
    return ostack+1;
}

/* java.lang.VMThread */

/* static method currentThread()Ljava/lang/Thread; */
uintptr_t *currentThread(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = (uintptr_t)getExecEnv()->thread;
    return ostack;
}

/* static method create(Ljava/lang/Thread;J)V */
uintptr_t *create(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *this = (Object *)ostack[0];
    long long stack_size = *((long long*)&ostack[1]);
    createJavaThread(this, stack_size);
    return ostack;
}

/* static method sleep(JI)V */
uintptr_t *jamSleep(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    long long ms = *((long long *)&ostack[0]);
    int ns = ostack[2];
    Thread *thread = threadSelf();

    threadSleep(thread, ms, ns);
    return ostack;
}

/* instance method interrupt()V */
uintptr_t *interrupt(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *this = (Object *)*ostack;
    Thread *thread = threadSelf0(this);
    if(thread)
        threadInterrupt(thread);
    return ostack;
}

/* instance method isAlive()Z */
uintptr_t *isAlive(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *this = (Object *)*ostack;
    Thread *thread = threadSelf0(this);
    *ostack++ = thread ? threadIsAlive(thread) : FALSE;
    return ostack;
}

/* static method yield()V */
uintptr_t *yield(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Thread *thread = threadSelf();
    threadYield(thread);
    return ostack;
}

/* instance method isInterrupted()Z */
uintptr_t *isInterrupted(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *this = (Object *)*ostack;
    Thread *thread = threadSelf0(this);
    *ostack++ = thread ? threadIsInterrupted(thread) : FALSE;
    return ostack;
}

/* static method interrupted()Z */
uintptr_t *interrupted(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Thread *thread = threadSelf();
    *ostack++ = threadInterrupted(thread);
    return ostack;
}

/* instance method nativeSetPriority(I)V */
uintptr_t *nativeSetPriority(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    return ostack+1;
}

/* instance method holdsLock(Ljava/lang/Object;)Z */
uintptr_t *holdsLock(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *ob = (Object *)ostack[0];
    if(ob == NULL)
        signalException("java/lang/NullPointerException", NULL);
    else
        *ostack++ = objectLockedByCurrent(ob);
    return ostack;
}

/* instance method getState()Ljava/lang/String; */
uintptr_t *getState(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Object *this = (Object *)*ostack;
    Thread *thread = threadSelf0(this);
    char *state = thread ? getThreadStateString(thread) : "TERMINATED";

    *ostack++ = (uintptr_t)Cstr2String(state);
    return ostack;
}

/* java.security.VMAccessController */

/* instance method getStack()[[Ljava/lang/Object; */
uintptr_t *getStack(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    Class *object_class = findArrayClass("[[Ljava/lang/Object;");
    Class *class_class = findArrayClass("[Ljava/lang/Class;");
    Class *string_class = findArrayClass("[Ljava/lang/String;");
    Object *stack, *names, *classes;
    Frame *frame;
    int depth;

    if(object_class == NULL || class_class == NULL || string_class == NULL)
      return ostack;

    frame = getExecEnv()->last_frame;
    depth = 0;

    do {
        for(; frame->mb != NULL; frame = frame->prev, depth++);
    } while((frame = frame->prev)->prev != NULL);

    stack = allocArray(object_class, 2, sizeof(Object*));
    classes = allocArray(class_class, depth, sizeof(Object*));
    names = allocArray(string_class, depth, sizeof(Object*));

    if(stack != NULL && names != NULL && classes != NULL) {
        Class **dcl = ARRAY_DATA(classes);
        Object **dnm = ARRAY_DATA(names);
        Object **stk = ARRAY_DATA(stack);

        frame = getExecEnv()->last_frame;

        do {
            for(; frame->mb != NULL; frame = frame->prev) {
                *dcl++ = frame->mb->class;
                *dnm++ = createString(frame->mb->name);
            }
        } while((frame = frame->prev)->prev != NULL);

        stk[0] = classes;
        stk[1] = names;
    }

    *ostack++ = (uintptr_t) stack;
    return ostack;
}

uintptr_t *getThreadCount(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = getThreadsCount();
    return ostack;
}

uintptr_t *getPeakThreadCount(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = getPeakThreadsCount();
    return ostack;
}

uintptr_t *getTotalStartedThreadCount(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *(u8*)ostack = getTotalStartedThreadsCount();
    return ostack + 2;
}

uintptr_t *resetPeakThreadCount(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    resetPeakThreadsCount();
    return ostack;
}

uintptr_t *findMonitorDeadlockedThreads(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    *ostack++ = (uintptr_t)NULL;
    return ostack;
}

uintptr_t *getThreadInfoForId(Class *class, MethodBlock *mb, uintptr_t *ostack) {
    long long id = *((long long *)&ostack[0]);
    int max_depth = ostack[2];

    Thread *thread = findThreadById(id);
    Object *info = NULL;

    if(thread != NULL) {
        Class *info_class = findSystemClass("java/lang/management/ThreadInfo");

        if(info_class != NULL) {
            MethodBlock *init = findMethod(info_class, "<init>",
                                           "(Ljava/lang/Thread;JJLjava/lang/Object;"
                                           "Ljava/lang/Thread;JJZZ[Ljava/lang/StackTraceElement;)V");
            if(init != NULL) {
                Frame *last;
                int in_native;
                Object *vmthrowable;
                int self = thread == threadSelf();

                if(!self)
                    suspendThread(thread);

                vmthrowable = setStackTrace0(thread->ee, max_depth);

                last = thread->ee->last_frame;
                in_native = last->prev == NULL || last->mb->access_flags & ACC_NATIVE;

                if(!self)
                    resumeThread(thread);

                if(vmthrowable != NULL) {
                    Object *trace;
                    if((info = allocObject(info_class)) != NULL &&
                               (trace = convertStackTrace(vmthrowable)) != NULL) {

                        Monitor *mon = thread->blocked_mon;
                        Object *lock = mon != NULL ? mon->obj : NULL;
                        Thread *owner = mon != NULL ? mon->owner : NULL;
                        Object *lock_owner = owner != NULL ? owner->ee->thread : NULL;

                        executeMethod(info, init, thread->ee->thread, thread->blocked_count, 0LL, lock,
                                      lock_owner, thread->waited_count, 0LL, in_native, FALSE, trace);
                    }
                }
            }
        }
    }

    *ostack++ = (uintptr_t)info;
    return ostack;
}

VMMethod vm_object[] = {
    {"getClass",                    getClass},
    {"clone",                       jamClone},
    {"wait",                        jamWait},
    {"notify",                      notify},
    {"notifyAll",                   notifyAll},
    {NULL,                          NULL}
};

VMMethod vm_system[] = {
    {"arraycopy",                   arraycopy},
    {"identityHashCode",            identityHashCode},
    {NULL,                          NULL}
};

VMMethod vm_runtime[] = {
    {"availableProcessors",         availableProcessors},
    {"freeMemory",                  freeMemory},
    {"totalMemory",                 totalMemory},
    {"maxMemory",                   maxMemory},
    {"gc",                          gc},
    {"runFinalization",             runFinalization},
    {"exit",                        exitInternal},
    {"nativeLoad",                  nativeLoad},
    {"mapLibraryName",              mapLibraryName},
    {NULL,                          NULL}
};

VMMethod vm_class[] = {
    {"isInstance",                  isInstance},
    {"isAssignableFrom",            isAssignableFrom},
    {"isInterface",                 isInterface},
    {"isPrimitive",                 isPrimitive},
    {"isArray",                     isArray},
    {"isMemberClass",               isMember},
    {"isLocalClass",                isLocal},
    {"isAnonymousClass",            isAnonymous},
    {"getEnclosingClass",           getEnclosingClass0},
    {"getEnclosingMethod",          getEnclosingMethod0},
    {"getEnclosingConstructor",     getEnclosingConstructor},
    {"getClassSignature",           getClassSignature},
    {"getSuperclass",               getSuperclass},
    {"getComponentType",            getComponentType},
    {"getName",                     getName},
    {"getDeclaredClasses",          getDeclaredClasses},
    {"getDeclaringClass",           getDeclaringClass0},
    {"getDeclaredConstructors",     getDeclaredConstructors},
    {"getDeclaredMethods",          getDeclaredMethods},
    {"getDeclaredFields",           getDeclaredFields},
    {"getInterfaces",               getInterfaces},
    {"getClassLoader",              getClassLoader},
    {"getModifiers",                getClassModifiers},
    {"forName",                     forName},
    {"throwException",              throwException},
    {"hasClassInitializer",         hasClassInitializer},
    {NULL,                          NULL}
};

VMMethod vm_string[] = {
    {"intern",                      intern},
    {NULL,                          NULL}
};

VMMethod vm_thread[] = {
    {"currentThread",               currentThread},
    {"create",                      create},
    {"sleep",                       jamSleep},
    {"interrupt",                   interrupt},
    {"isAlive",                     isAlive},
    {"yield",                       yield},
    {"isInterrupted",               isInterrupted},
    {"interrupted",                 interrupted},
    {"nativeSetPriority",           nativeSetPriority},
    {"holdsLock",                   holdsLock},
    {"getState",                    getState},
    {NULL,                          NULL}
};

VMMethod vm_throwable[] = {
    {"fillInStackTrace",            fillInStackTrace},
    {"getStackTrace",               getStackTrace},
    {NULL,                          NULL}
};

VMMethod vm_classloader[] = {
    {"loadClass",                   loadClass},
    {"getPrimitiveClass",           getPrimitiveClass},
    {"defineClass",                 defineClass0},
    {"findLoadedClass",             findLoadedClass},
    {"resolveClass",                resolveClass0},
    {"getBootClassPathSize",        getBootClassPathSize},
    {"getBootClassPathResource",    getBootClassPathResource},
    {NULL,                          NULL}
};

VMMethod vm_reflect_constructor[] = {
    {"constructNative",             constructNative},
    {"getConstructorModifiers",     getMethodModifiers},
    {"getSignature",                getMethodSignature},
    {NULL,                          NULL}
};

VMMethod vm_reflect_method[] = {
    {"invokeNative",                invokeNative},
    {"getMethodModifiers",          getMethodModifiers},
    {"getSignature",                getMethodSignature},
    {NULL,                          NULL}
};

VMMethod vm_reflect_field[] = {
    {"getFieldModifiers",           getFieldModifiers},
    {"getSignature",                getFieldSignature},
    {"getField",                    getField},
    {"setField",                    setField},
    {"setZField",                   setPrimitiveField},
    {"setBField",                   setPrimitiveField},
    {"setCField",                   setPrimitiveField},
    {"setSField",                   setPrimitiveField},
    {"setIField",                   setPrimitiveField},
    {"setFField",                   setPrimitiveField},
    {"setJField",                   setPrimitiveField},
    {"setDField",                   setPrimitiveField},
    {"getZField",                   getPrimitiveField},
    {"getBField",                   getPrimitiveField},
    {"getCField",                   getPrimitiveField},
    {"getSField",                   getPrimitiveField},
    {"getIField",                   getPrimitiveField},
    {"getFField",                   getPrimitiveField},
    {"getJField",                   getPrimitiveField},
    {"getDField",                   getPrimitiveField},
    {NULL,                          NULL}
};

VMMethod vm_system_properties[] = {
    {"preInit",                     propertiesPreInit},
    {"postInit",                    propertiesPostInit},
    {NULL,                          NULL}
};

VMMethod vm_stack_walker[] = {
    {"getClassContext",             getClassContext},
    {"getCallingClass",             getCallingClass},
    {"getCallingClassLoader",       getCallingClassLoader},
    {"firstNonNullClassLoader",     firstNonNullClassLoader},
    {NULL,                          NULL}
};

VMMethod vm_access_controller[] = {
    {"getStack",                    getStack},
    {NULL,                          NULL}
};

VMMethod vm_threadmx_bean_impl[] = {
    {"getThreadCount",              getThreadCount},
    {"getPeakThreadCount",          getPeakThreadCount},
    {"getTotalStartedThreadCount",  getTotalStartedThreadCount},
    {"resetPeakThreadCount",        resetPeakThreadCount},
    {"getThreadInfoForId",          getThreadInfoForId},
    {"findMonitorDeadlockedThreads",findMonitorDeadlockedThreads},
    {NULL,                          NULL}
};

VMClass native_methods[] = {
    {"java/lang/VMClass",                           vm_class},
    {"java/lang/VMObject",                          vm_object},
    {"java/lang/VMThread",                          vm_thread},
    {"java/lang/VMSystem",                          vm_system},
    {"java/lang/VMString",                          vm_string},
    {"java/lang/VMRuntime",                         vm_runtime},
    {"java/lang/VMThrowable",                       vm_throwable},
    {"java/lang/VMClassLoader",                     vm_classloader},
    {"java/lang/reflect/Field",                     vm_reflect_field},
    {"java/lang/reflect/Method",                    vm_reflect_method},
    {"java/lang/reflect/Constructor",               vm_reflect_constructor},
    {"java/security/VMAccessController",            vm_access_controller},
    {"gnu/classpath/VMSystemProperties",            vm_system_properties},
    {"gnu/classpath/VMStackWalker",                 vm_stack_walker},
    {"gnu/java/lang/management/VMThreadMXBeanImpl", vm_threadmx_bean_impl},
    {NULL,                                          NULL}
};
