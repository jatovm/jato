#ifndef JATO__JAVA_LANG_VMCLASS_H
#define JATO__JAVA_LANG_VMCLASS_H

#include "vm/jni.h"

jobject		java_lang_VMClass_forName(jobject name, jboolean initialize, jobject loader);
jobject		java_lang_VMClass_getClassLoader(jobject object);
jobject		java_lang_VMClass_getComponentType(jobject object);
jint		java_lang_VMClass_getModifiers(jobject clazz);
jobject		java_lang_VMClass_getName(jobject object);
jboolean	java_lang_VMClass_isAnonymousClass(jobject object);
jboolean	java_lang_VMClass_isArray(jobject object);
jboolean	java_lang_VMClass_isAssignableFrom(jobject clazz_1, jobject clazz_2);
jboolean	java_lang_VMClass_isInstance(jobject clazz, jobject object);
jboolean	java_lang_VMClass_isInterface(jobject clazz);
jboolean	java_lang_VMClass_isPrimitive(jobject object);

#endif /* JATO__JAVA_LANG_VMCLASS_H */
