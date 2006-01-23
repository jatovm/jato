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

#include <stdio.h>
#include <string.h>
#include "jam.h"

MethodBlock *findMethod(Class *class, char *methodname, char *type) {
   ClassBlock *cb = CLASS_CB(class);
   MethodBlock *mb = cb->methods;
   int i;

   for(i = 0; i < cb->methods_count; i++,mb++)
       if((strcmp(mb->name, methodname) == 0) && (strcmp(mb->type, type) == 0))
          return mb;

   return NULL;
}

/* As a Java program can't have two fields with the same name but different types,
   we used to give up if we found a field with the right name but wrong type.
   However, obfuscators rename fields, breaking this optimisation.
*/
FieldBlock *findField(Class *class, char *fieldname, char *type) {
    ClassBlock *cb = CLASS_CB(class);
    FieldBlock *fb = cb->fields;
    int i;

    for(i = 0; i < cb->fields_count; i++,fb++)
        if(strcmp(fb->name, fieldname) == 0 && (strcmp(fb->type, type) == 0))
            return fb;

    return NULL;
}

MethodBlock *lookupMethod(Class *class, char *methodname, char *type) {
    MethodBlock *mb;

    if((mb = findMethod(class, methodname, type)))
       return mb;

    if(CLASS_CB(class)->super)
        return lookupMethod(CLASS_CB(class)->super, methodname, type);

    return NULL;
}

FieldBlock *lookupField(Class *class, char *fieldname, char *fieldtype) {
    ClassBlock *cb;
    FieldBlock *fb;
    int i;

    if((fb = findField(class, fieldname, fieldtype)) != NULL)
        return fb;

    cb = CLASS_CB(class);
    i = cb->super ? CLASS_CB(cb->super)->imethod_table_size : 0;

    for(; i < cb->imethod_table_size; i++) {
        Class *intf = cb->imethod_table[i].interface;
        if((fb = findField(intf, fieldname, fieldtype)) != NULL)
            return fb;
    }

    if(cb->super)
        return lookupField(cb->super, fieldname, fieldtype);

    return NULL;
}

Class *resolveClass(Class *class, int cp_index, int init) {
    ConstantPool *cp = &(CLASS_CB(class)->constant_pool);
    Class *resolved_class;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            resolved_class = (Class *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_Class: {
            char *classname;
            int name_idx = CP_CLASS(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Class)
                goto retry;

            classname = CP_UTF8(cp, name_idx);
            resolved_class = findClassFromClass(classname, class);

            /* If we can't find the class an exception will already have
               been thrown */

            if(resolved_class == NULL)
                return NULL;

            if(!checkClassAccess(resolved_class, class)) {
                signalException("java/lang/IllegalAccessException", "class is not accessible");
                return NULL;
            }

            CP_TYPE(cp, cp_index) = CONSTANT_Locked;
            MBARRIER();
            CP_INFO(cp, cp_index) = (uintptr_t)resolved_class;
            MBARRIER();
            CP_TYPE(cp, cp_index) = CONSTANT_Resolved;

            break;
        }
    }

    if(init)
        initClass(resolved_class);

    return resolved_class;
}

MethodBlock *resolveMethod(Class *class, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(class)->constant_pool);
    MethodBlock *mb;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            mb = (MethodBlock *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_Methodref: {
            Class *resolved_class;
            ClassBlock *resolved_cb;
            char *methodname, *methodtype;
            int cl_idx = CP_METHOD_CLASS(cp, cp_index);
            int name_type_idx = CP_METHOD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Methodref)
                goto retry;

            methodname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, name_type_idx));
            methodtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            resolved_class = resolveClass(class, cl_idx, FALSE);
            resolved_cb = CLASS_CB(resolved_class);

            if(exceptionOccured())
                return NULL;

            if(resolved_cb->access_flags & ACC_INTERFACE) {
                signalException("java/lang/IncompatibleClassChangeError", NULL);
                return NULL;
            }
            
            mb = lookupMethod(resolved_class, methodname, methodtype);

            if(mb) {
                if((mb->access_flags & ACC_ABSTRACT) &&
                       !(resolved_cb->access_flags & ACC_ABSTRACT)) {
                    signalException("java/lang/AbstractMethodError", methodname);
                    return NULL;
                }

                if(!checkMethodAccess(mb, class)) {
                    signalException("java/lang/IllegalAccessException", "method is not accessible");
                    return NULL;
                }

                initClass(mb->class);

                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)mb;
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            } else
                signalException("java/lang/NoSuchMethodError", methodname);

            break;
        }
    }

    return mb;
}

MethodBlock *resolveInterfaceMethod(Class *class, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(class)->constant_pool);
    MethodBlock *mb;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            mb = (MethodBlock *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_InterfaceMethodref: {
            Class *resolved_class;
            char *methodname, *methodtype;
            int cl_idx = CP_METHOD_CLASS(cp, cp_index);
            int name_type_idx = CP_METHOD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_InterfaceMethodref)
                goto retry;

            methodname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, name_type_idx));
            methodtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            resolved_class = resolveClass(class, cl_idx, FALSE);

            if(exceptionOccured())
                return NULL;

            if(!(CLASS_CB(resolved_class)->access_flags & ACC_INTERFACE)) {
                signalException("java/lang/IncompatibleClassChangeError", NULL);
                return NULL;
            }
            
            if((mb = lookupMethod(resolved_class, methodname, methodtype)) == NULL) {
                ClassBlock *cb = CLASS_CB(resolved_class);
                int i;

                for(i = 0; mb == NULL && (i < cb->imethod_table_size); i++) {
                    Class *intf = cb->imethod_table[i].interface;
                    mb = findMethod(intf, methodname, methodtype);
                }
            }

            if(mb) {
                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)mb;
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            } else
                signalException("java/lang/NoSuchMethodError", methodname);

            break;
        }
    }

    return mb;
}

FieldBlock *resolveField(Class *class, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(class)->constant_pool);
    FieldBlock *fb;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            fb = (FieldBlock *)CP_INFO(cp, cp_index);
            break;

        case CONSTANT_Fieldref: {
            Class *resolved_class;
            char *fieldname, *fieldtype;
            int cl_idx = CP_FIELD_CLASS(cp, cp_index);
            int name_type_idx = CP_FIELD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Fieldref)
                goto retry;

            fieldname = CP_UTF8(cp, CP_NAME_TYPE_NAME(cp, name_type_idx));
            fieldtype = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            resolved_class = resolveClass(class, cl_idx, FALSE);

            if(exceptionOccured())
                return NULL;

            fb = lookupField(resolved_class, fieldname, fieldtype);

            if(fb) {
                if(!checkFieldAccess(fb, class)) {
                    signalException("java/lang/IllegalAccessException", "field is not accessible");
                    return NULL;
                }

                initClass(fb->class);

                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)fb;
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            } else
                signalException("java/lang/NoSuchFieldError", fieldname);

            break;
        }
    }
 
    return fb;
}

uintptr_t resolveSingleConstant(Class *class, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(class)->constant_pool);

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Class:
            resolveClass(class, cp_index, FALSE);
            break;

        case CONSTANT_String: {
            Object *string;
            int idx = CP_STRING(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_String)
                goto retry;

            string = createString(CP_UTF8(cp, idx));

            if(string) {
                CP_TYPE(cp, cp_index) = CONSTANT_Locked;
                MBARRIER();
                CP_INFO(cp, cp_index) = (uintptr_t)findInternedString(string);
                MBARRIER();
                CP_TYPE(cp, cp_index) = CONSTANT_Resolved;
            }

            break;
        }

        default:
            break;
    }
    
    return CP_INFO(cp, cp_index);
}

MethodBlock *lookupVirtualMethod(Object *ob, MethodBlock *mb) {
    ClassBlock *cb = CLASS_CB(ob->class);
    int mtbl_idx = mb->method_table_index;

    if(mb->access_flags & ACC_PRIVATE)
        return mb;

    if(CLASS_CB(mb->class)->access_flags & ACC_INTERFACE) {
        int i;

        for(i = 0; (i < cb->imethod_table_size) &&
                   (mb->class != cb->imethod_table[i].interface); i++);

        if(i == cb->imethod_table_size) {
            signalException("java/lang/IncompatibleClassChangeError",
                            "unimplemented interface");
            return NULL;
        }
        mtbl_idx = cb->imethod_table[i].offsets[mtbl_idx];
    }

    mb = cb->method_table[mtbl_idx];

    if(mb->access_flags & ACC_ABSTRACT) {
        signalException("java/lang/AbstractMethodError", NULL);
        return NULL;
    }

    return mb;
}

/* This function is used when rewriting a field access bytecode
   in the direct-threaded interpreter.  We need to know how many
   slots are used on the stack, but the field reference may not
   be resolved, and resolving at preparation will break Java's
   lazy resolution semantics. */

#ifdef DIRECT
int peekIsFieldLong(Class *class, int cp_index) {
    ConstantPool *cp = &(CLASS_CB(class)->constant_pool);
    char *type;

retry:
    switch(CP_TYPE(cp, cp_index)) {
        case CONSTANT_Locked:
            goto retry;

        case CONSTANT_Resolved:
            type = ((FieldBlock *)CP_INFO(cp, cp_index))->type;
            break;

        case CONSTANT_Fieldref: {
            int name_type_idx = CP_FIELD_NAME_TYPE(cp, cp_index);

            if(CP_TYPE(cp, cp_index) != CONSTANT_Fieldref)
                goto retry;

            type = CP_UTF8(cp, CP_NAME_TYPE_TYPE(cp, name_type_idx));
            break;
        }
    }
 
    return *type == 'J' || *type == 'D';
}
#endif
