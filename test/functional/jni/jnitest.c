#include <jni.h>

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassed
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_JNITest_returnPassed(JNIEnv *env, jclass clazz, jstring jstr)
{
	return jstr;
}
