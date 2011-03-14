#include <string.h>
#include <stdio.h>
#include <vm/jni.h>

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
 * Method:    returnPassedStringArray
 * Signature: ([Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_java_lang_JNITest_returnPassedStringArray(JNIEnv *env, jobject jobj, jobjectArray objectArrayValue)
{
	return objectArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedIntArray
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_java_lang_JNITest_returnPassedIntArray(JNIEnv *env, jobject jobj, jintArray intArrayValue)
{
	return intArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedLongArray
 * Signature: ([J)[J
 */
JNIEXPORT jlongArray JNICALL Java_java_lang_JNITest_returnPassedLongArray(JNIEnv *env, jobject jobj, jlongArray longArrayValue)
{
	return longArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedBooleanArray
 * Signature: ([Z)[Z
 */
JNIEXPORT jbooleanArray JNICALL Java_java_lang_JNITest_returnPassedBooleanArray(JNIEnv *env, jobject jobj, jbooleanArray booleanArrayValue)
{
	return booleanArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedShortArray
 * Signature: ([S)[S
 */
JNIEXPORT jshortArray JNICALL Java_java_lang_JNITest_returnPassedShortArray(JNIEnv *env, jobject jobj, jshortArray shortArrayValue)
{
	return shortArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedByteArray
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_java_lang_JNITest_returnPassedByteArray(JNIEnv *env, jobject jobj, jbyteArray byteArrayValue)
{
	return byteArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedCharArray
 * Signature: ([C)[C
 */
JNIEXPORT jcharArray JNICALL Java_java_lang_JNITest_returnPassedCharArray(JNIEnv *env, jobject jobj, jcharArray charArrayValue)
{
	return charArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedFloatArray
 * Signature: ([F)[F
 */
JNIEXPORT jfloatArray JNICALL Java_java_lang_JNITest_returnPassedFloatArray(JNIEnv *env, jobject jobj, jfloatArray floatArrayValue)
{
	return floatArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    returnPassedDoubleArray
 * Signature: ([D)[D
 */
JNIEXPORT jdoubleArray JNICALL Java_java_lang_JNITest_returnPassedDoubleArray(JNIEnv *env, jobject jobj, jdoubleArray doubleArrayValue)
{
	return doubleArrayValue;
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

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedStringArray
 * Signature: ([Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_java_lang_JNITest_staticReturnPassedStringArray(JNIEnv *env, jclass clazz, jobjectArray stringArrayValue)
{
	return stringArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedIntArray
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_java_lang_JNITest_staticReturnPassedIntArray(JNIEnv *env, jclass clazz, jintArray intArrayValue)
{
	return intArrayValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedLongArray
 * Signature: ([J)[J
 */
JNIEXPORT jlongArray JNICALL Java_java_lang_JNITest_staticReturnPassedLongArray(JNIEnv *env, jclass clazz, jlongArray longArrayValue)
{
	return longArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedBooleanArray
 * Signature: ([Z)[Z
 */
JNIEXPORT jbooleanArray JNICALL Java_java_lang_JNITest_staticReturnPassedBooleanArray(JNIEnv *env, jclass clazz, jbooleanArray booleanArrayValue)
{
	return booleanArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedShortArray
 * Signature: ([S)[S
 */
JNIEXPORT jshortArray JNICALL Java_java_lang_JNITest_staticReturnPassedShortArray(JNIEnv *env, jclass clazz, jshortArray shortArrayValue)
{
	return shortArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedByteArray
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_java_lang_JNITest_staticReturnPassedByteArray(JNIEnv *env, jclass clazz, jbyteArray byteArrayValue)
{
	return byteArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedCharArray
 * Signature: ([C)[C
 */
JNIEXPORT jcharArray JNICALL Java_java_lang_JNITest_staticReturnPassedCharArray(JNIEnv *env, jclass clazz, jcharArray charArrayValue)
{
	return charArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedFloatArray
 * Signature: ([F)[F
 */
JNIEXPORT jfloatArray JNICALL Java_java_lang_JNITest_staticReturnPassedFloatArray(JNIEnv *env, jclass clazz, jfloatArray floatArrayValue)
{
	return floatArrayValue;
}


/*
 * Class:     java_lang_JNITest
 * Method:    staticReturnPassedDoubleArray
 * Signature: ([D)[D
 */
JNIEXPORT jdoubleArray JNICALL Java_java_lang_JNITest_staticReturnPassedDoubleArray(JNIEnv *env, jclass clazz, jdoubleArray doubleArrayValue)
{
	return doubleArrayValue;
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticToUpper
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_java_lang_JNITest_staticToUpper(JNIEnv *env, jclass clazz, jstring stringValue)
{
	char upperStr[128];
	const jbyte *str;
	jboolean iscopy;

	str = (*env)->GetStringUTFChars(env, stringValue, &iscopy);
	if (str == NULL) {
		return NULL; /* OutOfMemoryError */
	}

	memset(upperStr, '\0', 128);

	int i;

	for(i=0; str[i] || i >= 128; i++)
		upperStr[i] = toupper(str[i]);

	(*env)->ReleaseStringUTFChars(env, stringValue, str);

	return (*env)->NewStringUTF(env, upperStr);
}

/*
 * Class:     java_lang_JNITest
 * Method:    staticGetVersion
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_java_lang_JNITest_staticGetVersion(JNIEnv *env, jclass clazz)
{
	return (*env)->GetVersion(env);
}
