#ifndef JATO__JAVA_LANG_VMTHREAD_H
#define JATO__JAVA_LANG_VMTHREAD_H

#include "vm/jni.h"

jobject java_lang_VMThread_currentThread(void);
void java_lang_VMThread_start(jobject vmthread, jlong stacksize);
void java_lang_VMThread_yield(void);
jboolean java_lang_VMThread_interrupted(void);
jboolean java_lang_VMThread_isInterrupted(jobject vmthread);
void java_lang_VMThread_setPriority(jobject vmthread, jint ptiority);
void java_lang_VMThread_interrupt(jobject vmthread);

#endif /* JATO__JAVA_LANG_VMTHREAD_H */
