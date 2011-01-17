#include <jni.h>

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassed
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_JNITest_returnPassedString(JNIEnv *env, jclass clazz, jstring stringValue)
{
	return stringValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedInt
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_java_lang_JNITest_returnPassedInt(JNIEnv *env, jclass clazz, jint intValue)
{
	return intValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_java_lang_JNITest_returnPassedLong(JNIEnv *env, jclass clazz, jlong longValue)
{
	return longValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedBoolean
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_JNITest_returnPassedBoolean(JNIEnv *env, jclass clazz, jboolean booleanValue)
{
	return booleanValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedShort
 * Signature: (S)S
 */
JNIEXPORT jshort JNICALL Java_java_lang_JNITest_returnPassedShort(JNIEnv *env, jclass clazz, jshort shortValue)
{
	return shortValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedByte
 * Signature: (B)B
 */
JNIEXPORT jbyte JNICALL Java_java_lang_JNITest_returnPassedByte(JNIEnv *env, jclass clazz, jbyte byteValue)
{
	return byteValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedChar
 * Signature: (C)C
 */
JNIEXPORT jchar JNICALL Java_java_lang_JNITest_returnPassedChar(JNIEnv *env, jclass clazz, jchar charValue)
{
	return charValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedFloat
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_java_lang_JNITest_returnPassedFloat(JNIEnv *env, jclass clazz, jfloat floatValue)
{
	return floatValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedDouble
 * Signature: (D)D
 */
JNIEXPORT jdouble JNICALL Java_java_lang_JNITest_returnPassedDouble(JNIEnv *env, jclass clazz, jdouble doubleValue)
{
	return doubleValue;
}
