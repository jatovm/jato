#ifndef RUNTIME_CLASSLOADER_H
#define RUNTIME_CLASSLOADER_H

#include "vm/jni.h"

jobject native_vmclassloader_getprimitiveclass(jint type);
jobject native_vmclassloader_findloadedclass(jobject classloader, jobject name);
jobject native_vmclassloader_loadclass(jobject name, jboolean resolve);

#endif /* RUNTIME_CLASSLOADER_H */
