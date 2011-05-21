#ifndef JATO__JAVA_LANG_VMCLASSLOADER_H
#define JATO__JAVA_LANG_VMCLASSLOADER_H

#include "vm/jni.h"

jobject native_vmclassloader_getprimitiveclass(jint type);
jobject native_vmclassloader_findloadedclass(jobject classloader, jobject name);
jobject native_vmclassloader_loadclass(jobject name, jboolean resolve);
jobject native_vmclassloader_defineclass(jobject classloader, jobject name, jobject data, jint offset, jint len, jobject pd);
void native_vmclassloader_resolveclass(jobject classloader, jobject clazz);

#endif /* JATO__JAVA_LANG_VMCLASSLOADER_H */
