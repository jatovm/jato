#ifndef JATO__JAVA_LANG_REFLECT_VMMETHOD_H
#define JATO__JAVA_LANG_REFLECT_VMMETHOD_H

#include "vm/jni.h"

jobject		java_lang_reflect_VMMethod_getAnnotation(jobject this, jobject annotation);
jobject         java_lang_reflect_VMMethod_getDeclaredAnnotations(jobject this);

#endif /* JATO__JAVA_LANG_REFLECT_VMMETHOD_H */
