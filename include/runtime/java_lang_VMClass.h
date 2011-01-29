#ifndef JATO__JAVA_LANG_VMCLASS_H
#define JATO__JAVA_LANG_VMCLASS_H

#include "vm/jni.h"

jobject		java_lang_VMClass_forName(jobject name, jboolean initialize, jobject loader);
jobject		java_lang_VMClass_getClassLoader(jobject object);
jobject		java_lang_VMClass_getComponentType(jobject object);
jobject		java_lang_VMClass_getDeclaredAnnotations(jobject klass);
jobject		java_lang_VMClass_getDeclaredConstructors(jobject class_object, jboolean public_only);
jobject		java_lang_VMClass_getDeclaredFields(jobject class_object, jboolean public_only);
jobject		java_lang_VMClass_getDeclaredMethods(jobject class_object, jboolean public_only);
jobject		java_lang_VMClass_getDeclaringClass(jobject clazz);
jobject		java_lang_VMClass_getEnclosingClass(jobject clazz);
jobject		java_lang_VMClass_getInterfaces(jobject clazz);
jint		java_lang_VMClass_getModifiers(jobject clazz, jboolean ignore_inner_class_attrib);
jobject		java_lang_VMClass_getName(jobject object);
jobject		java_lang_VMClass_getSuperclass(jobject clazz);
jboolean	java_lang_VMClass_isAnonymousClass(jobject object);
jboolean	java_lang_VMClass_isArray(jobject object);
jboolean	java_lang_VMClass_isAssignableFrom(jobject clazz_1, jobject clazz_2);
jboolean	java_lang_VMClass_isInstance(jobject clazz, jobject object);
jboolean	java_lang_VMClass_isInterface(jobject clazz);
jboolean	java_lang_VMClass_isLocalClass(jobject clazz);
jboolean	java_lang_VMClass_isMemberClass(jobject clazz);
jboolean	java_lang_VMClass_isPrimitive(jobject object);

#endif /* JATO__JAVA_LANG_VMCLASS_H */
