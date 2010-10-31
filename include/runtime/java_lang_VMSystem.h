#ifndef JATO__JAVA_LANG_VMSYSTEM_H
#define JATO__JAVA_LANG_VMSYSTEM_H

#include "vm/jni.h"

void native_vmsystem_arraycopy(jobject src, jint src_start, jobject dest, jint dest_start, jint len);
jint native_vmsystem_identityhashcode(jobject object);

#endif /* JATO__JAVA_LANG_VMSYSTEM_H */
