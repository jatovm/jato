#ifndef JATO__JAVA_LANG_REFLECT_VMFIELD_H
#define JATO__JAVA_LANG_REFLECT_VMFIELD_H

#include "vm/jni.h"

jobject		java_lang_reflect_VMField_getAnnotation(jobject this, jobject annotation);
jobject		java_lang_reflect_VMField_getDeclaredAnnotations(jobject this);

#endif /* JATO__JAVA_LANG_REFLECT_VMFIELD_H */
