#ifndef _VM_JAVA_LANG_H
#define _VM_JAVA_LANG_H

extern struct vm_class *vm_java_lang_Object;
extern struct vm_class *vm_java_lang_Class;
extern struct vm_class *vm_java_lang_String;
extern struct vm_class *vm_java_lang_Throwable;
extern struct vm_class *vm_boolean_class;
extern struct vm_class *vm_char_class;
extern struct vm_class *vm_float_class;
extern struct vm_class *vm_double_class;
extern struct vm_class *vm_byte_class;
extern struct vm_class *vm_short_class;
extern struct vm_class *vm_int_class;
extern struct vm_class *vm_long_class;

extern struct vm_field *vm_java_lang_Class_vmdata;
extern struct vm_field *vm_java_lang_String_offset;
extern struct vm_field *vm_java_lang_String_count;
extern struct vm_field *vm_java_lang_String_value;
extern struct vm_field *vm_java_lang_Throwable_detailMessage;

#endif
