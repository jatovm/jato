#include <jni.h>

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassed
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_JNITest_returnPassedString(JNIEnv *env, jobject jobj, jstring stringValue)
{
	return stringValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedInt
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_java_lang_JNITest_returnPassedInt(JNIEnv *env, jobject jobj, jint intValue)
{
	return intValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_java_lang_JNITest_returnPassedLong(JNIEnv *env, jobject jobj, jlong longValue)
{
	return longValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedBoolean
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_JNITest_returnPassedBoolean(JNIEnv *env, jobject jobj, jboolean booleanValue)
{
	return booleanValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedShort
 * Signature: (S)S
 */
JNIEXPORT jshort JNICALL Java_java_lang_JNITest_returnPassedShort(JNIEnv *env, jobject jobj, jshort shortValue)
{
	return shortValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedByte
 * Signature: (B)B
 */
JNIEXPORT jbyte JNICALL Java_java_lang_JNITest_returnPassedByte(JNIEnv *env, jobject jobj, jbyte byteValue)
{
	return byteValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedChar
 * Signature: (C)C
 */
JNIEXPORT jchar JNICALL Java_java_lang_JNITest_returnPassedChar(JNIEnv *env, jobject jobj, jchar charValue)
{
	return charValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedFloat
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_java_lang_JNITest_returnPassedFloat(JNIEnv *env, jobject jobj, jfloat floatValue)
{
	return floatValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedDouble
 * Signature: (D)D
 */
JNIEXPORT jdouble JNICALL Java_java_lang_JNITest_returnPassedDouble(JNIEnv *env, jobject jobj, jdouble doubleValue)
{
	return doubleValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedString
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_JNITest_staticReturnPassedString(JNIEnv *env, jclass clazz, jstring stringValue)
{
	return stringValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedInt
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_java_lang_JNITest_staticReturnPassedInt(JNIEnv *env, jclass clazz, jint intValue)
{
	return intValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_java_lang_JNITest_staticReturnPassedLong(JNIEnv *env, jclass clazz, jlong longValue)
{
	return longValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedBoolean
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_java_lang_JNITest_staticReturnPassedBoolean(JNIEnv *env, jclass clazz, jboolean booleanValue)
{
	return booleanValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedShort
 * Signature: (S)S
 */
JNIEXPORT jshort JNICALL Java_java_lang_JNITest_staticReturnPassedShort(JNIEnv *env, jclass clazz, jshort shortValue)
{
	return shortValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedByte
 * Signature: (B)B
 */
JNIEXPORT jbyte JNICALL Java_java_lang_JNITest_staticReturnPassedByte(JNIEnv *env, jclass clazz, jbyte byteValue)
{
	return byteValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedChar
 * Signature: (C)C
 */
JNIEXPORT jchar JNICALL Java_java_lang_JNITest_staticReturnPassedChar(JNIEnv *env, jclass clazz, jchar charValue)
{
	return charValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedFloat
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_java_lang_JNITest_staticReturnPassedFloat(JNIEnv *env, jclass clazz, jfloat floatValue)
{
	return floatValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedDouble
 * Signature: (D)D
 */
JNIEXPORT jdouble JNICALL Java_java_lang_JNITest_staticReturnPassedDouble(JNIEnv *env, jclass clazz, jdouble doubleValue)
{
	return doubleValue;
}
