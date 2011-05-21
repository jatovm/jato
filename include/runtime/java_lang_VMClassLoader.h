#ifndef JATO__JAVA_LANG_VMCLASSLOADER_H
#define JATO__JAVA_LANG_VMCLASSLOADER_H

#include "vm/jni.h"

jobject java_lang_VMClassLoader_getPrimitiveClass(jint type);
jobject java_lang_VMClassLoader_findLoadedClass(jobject classloader, jobject name);
jobject java_lang_VMClassLoader_loadClass(jobject name, jboolean resolve);
jobject java_lang_VMClassLoader_defineClass(jobject classloader, jobject name, jobject data, jint offset, jint len, jobject pd);
void java_lang_VMClassLoader_resolveClass(jobject classloader, jobject clazz);

#endif /* JATO__JAVA_LANG_VMCLASSLOADER_H */
