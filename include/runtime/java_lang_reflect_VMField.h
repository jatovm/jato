#ifndef JATO__JAVA_LANG_REFLECT_VMFIELD_H
#define JATO__JAVA_LANG_REFLECT_VMFIELD_H

#include "vm/jni.h"

jobject		java_lang_reflect_VMField_getAnnotation(jobject this, jobject annotation);
jobject		java_lang_reflect_VMField_getDeclaredAnnotations(jobject this);
jobject		java_lang_reflect_VMField_get(jobject this, jobject o);
jlong		java_lang_reflect_VMField_getLong(jobject this, jobject o);
jint		java_lang_reflect_VMField_getInt(jobject this, jobject o);
jshort		java_lang_reflect_VMField_getShort(jobject this, jobject o);
jdouble		java_lang_reflect_VMField_getDouble(jobject this, jobject o);
jfloat		java_lang_reflect_VMField_getFloat(jobject this, jobject o);
jbyte		java_lang_reflect_VMField_getByte(jobject this, jobject o);
jchar		java_lang_reflect_VMField_getChar(jobject this, jobject o);
jboolean	java_lang_reflect_VMField_getBoolean(jobject this, jobject o);
jint		java_lang_reflect_VMField_getModifiersInternal(jobject this);
jobject		java_lang_reflect_VMField_getType(jobject this);
void		java_lang_reflect_VMField_set(jobject this, jobject o, jobject value_obj);

#endif /* JATO__JAVA_LANG_REFLECT_VMFIELD_H */
