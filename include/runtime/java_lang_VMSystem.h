#ifndef JATO__JAVA_LANG_VMSYSTEM_H
#define JATO__JAVA_LANG_VMSYSTEM_H

#include "vm/jni.h"

void java_lang_VMSystem_arraycopy(jobject src, jint src_start, jobject dest, jint dest_start, jint len);
jint java_lang_VMSystem_identityHashCode(jobject object);

#endif /* JATO__JAVA_LANG_VMSYSTEM_H */
