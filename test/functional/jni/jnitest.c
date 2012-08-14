#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <vm/jni.h>
#include <vm/preload.h>

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassed
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_test_java_lang_JNITest_returnPassedString(JNIEnv *env, jobject jobj, jstring stringValue)
{
	return stringValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedInt
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_test_java_lang_JNITest_returnPassedInt(JNIEnv *env, jobject jobj, jint intValue)
{
	return intValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_test_java_lang_JNITest_returnPassedLong(JNIEnv *env, jobject jobj, jlong longValue)
{
	return longValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedBoolean
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_returnPassedBoolean(JNIEnv *env, jobject jobj, jboolean booleanValue)
{
	return booleanValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedShort
 * Signature: (S)S
 */
JNIEXPORT jshort JNICALL Java_test_java_lang_JNITest_returnPassedShort(JNIEnv *env, jobject jobj, jshort shortValue)
{
	return shortValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedByte
 * Signature: (B)B
 */
JNIEXPORT jbyte JNICALL Java_test_java_lang_JNITest_returnPassedByte(JNIEnv *env, jobject jobj, jbyte byteValue)
{
	return byteValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedChar
 * Signature: (C)C
 */
JNIEXPORT jchar JNICALL Java_test_java_lang_JNITest_returnPassedChar(JNIEnv *env, jobject jobj, jchar charValue)
{
	return charValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedFloat
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_test_java_lang_JNITest_returnPassedFloat(JNIEnv *env, jobject jobj, jfloat floatValue)
{
	return floatValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedDouble
 * Signature: (D)D
 */
JNIEXPORT jdouble JNICALL Java_test_java_lang_JNITest_returnPassedDouble(JNIEnv *env, jobject jobj, jdouble doubleValue)
{
	return doubleValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedStringArray
 * Signature: ([Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_test_java_lang_JNITest_returnPassedStringArray(JNIEnv *env, jobject jobj, jobjectArray objectArrayValue)
{
	return objectArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedIntArray
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_test_java_lang_JNITest_returnPassedIntArray(JNIEnv *env, jobject jobj, jintArray intArrayValue)
{
	return intArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedLongArray
 * Signature: ([J)[J
 */
JNIEXPORT jlongArray JNICALL Java_test_java_lang_JNITest_returnPassedLongArray(JNIEnv *env, jobject jobj, jlongArray longArrayValue)
{
	return longArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedBooleanArray
 * Signature: ([Z)[Z
 */
JNIEXPORT jbooleanArray JNICALL Java_test_java_lang_JNITest_returnPassedBooleanArray(JNIEnv *env, jobject jobj, jbooleanArray booleanArrayValue)
{
	return booleanArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedShortArray
 * Signature: ([S)[S
 */
JNIEXPORT jshortArray JNICALL Java_test_java_lang_JNITest_returnPassedShortArray(JNIEnv *env, jobject jobj, jshortArray shortArrayValue)
{
	return shortArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedByteArray
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_test_java_lang_JNITest_returnPassedByteArray(JNIEnv *env, jobject jobj, jbyteArray byteArrayValue)
{
	return byteArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedCharArray
 * Signature: ([C)[C
 */
JNIEXPORT jcharArray JNICALL Java_test_java_lang_JNITest_returnPassedCharArray(JNIEnv *env, jobject jobj, jcharArray charArrayValue)
{
	return charArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedFloatArray
 * Signature: ([F)[F
 */
JNIEXPORT jfloatArray JNICALL Java_test_java_lang_JNITest_returnPassedFloatArray(JNIEnv *env, jobject jobj, jfloatArray floatArrayValue)
{
	return floatArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    returnPassedDoubleArray
 * Signature: ([D)[D
 */
JNIEXPORT jdoubleArray JNICALL Java_test_java_lang_JNITest_returnPassedDoubleArray(JNIEnv *env, jobject jobj, jdoubleArray doubleArrayValue)
{
	return doubleArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedString
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_test_java_lang_JNITest_staticReturnPassedString(JNIEnv *env, jclass clazz, jstring stringValue)
{
	return stringValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedInt
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_test_java_lang_JNITest_staticReturnPassedInt(JNIEnv *env, jclass clazz, jint intValue)
{
	return intValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedLong
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_test_java_lang_JNITest_staticReturnPassedLong(JNIEnv *env, jclass clazz, jlong longValue)
{
	return longValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedBoolean
 * Signature: (Z)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_staticReturnPassedBoolean(JNIEnv *env, jclass clazz, jboolean booleanValue)
{
	return booleanValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedShort
 * Signature: (S)S
 */
JNIEXPORT jshort JNICALL Java_test_java_lang_JNITest_staticReturnPassedShort(JNIEnv *env, jclass clazz, jshort shortValue)
{
	return shortValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedByte
 * Signature: (B)B
 */
JNIEXPORT jbyte JNICALL Java_test_java_lang_JNITest_staticReturnPassedByte(JNIEnv *env, jclass clazz, jbyte byteValue)
{
	return byteValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedChar
 * Signature: (C)C
 */
JNIEXPORT jchar JNICALL Java_test_java_lang_JNITest_staticReturnPassedChar(JNIEnv *env, jclass clazz, jchar charValue)
{
	return charValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedFloat
 * Signature: (F)F
 */
JNIEXPORT jfloat JNICALL Java_test_java_lang_JNITest_staticReturnPassedFloat(JNIEnv *env, jclass clazz, jfloat floatValue)
{
	return floatValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedDouble
 * Signature: (D)D
 */
JNIEXPORT jdouble JNICALL Java_test_java_lang_JNITest_staticReturnPassedDouble(JNIEnv *env, jclass clazz, jdouble doubleValue)
{
	return doubleValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedStringArray
 * Signature: ([Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jobjectArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedStringArray(JNIEnv *env, jclass clazz, jobjectArray stringArrayValue)
{
	return stringArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedIntArray
 * Signature: ([I)[I
 */
JNIEXPORT jintArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedIntArray(JNIEnv *env, jclass clazz, jintArray intArrayValue)
{
	return intArrayValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedLongArray
 * Signature: ([J)[J
 */
JNIEXPORT jlongArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedLongArray(JNIEnv *env, jclass clazz, jlongArray longArrayValue)
{
	return longArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedBooleanArray
 * Signature: ([Z)[Z
 */
JNIEXPORT jbooleanArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedBooleanArray(JNIEnv *env, jclass clazz, jbooleanArray booleanArrayValue)
{
	return booleanArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedShortArray
 * Signature: ([S)[S
 */
JNIEXPORT jshortArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedShortArray(JNIEnv *env, jclass clazz, jshortArray shortArrayValue)
{
	return shortArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedByteArray
 * Signature: ([B)[B
 */
JNIEXPORT jbyteArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedByteArray(JNIEnv *env, jclass clazz, jbyteArray byteArrayValue)
{
	return byteArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedCharArray
 * Signature: ([C)[C
 */
JNIEXPORT jcharArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedCharArray(JNIEnv *env, jclass clazz, jcharArray charArrayValue)
{
	return charArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedFloatArray
 * Signature: ([F)[F
 */
JNIEXPORT jfloatArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedFloatArray(JNIEnv *env, jclass clazz, jfloatArray floatArrayValue)
{
	return floatArrayValue;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    staticReturnPassedDoubleArray
 * Signature: ([D)[D
 */
JNIEXPORT jdoubleArray JNICALL Java_test_java_lang_JNITest_staticReturnPassedDoubleArray(JNIEnv *env, jclass clazz, jdoubleArray doubleArrayValue)
{
	return doubleArrayValue;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    staticToUpper
 * Signature: (Ljava/lang/String;)[Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_test_java_lang_JNITest_staticToUpper(JNIEnv *env, jclass clazz, jstring stringValue)
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
 * Class:     test_java_lang_JNITest
 * Method:    staticTestJNIEnvIsInitializedCorrectly
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_staticTestJNIEnvIsInitializedCorrectly(JNIEnv *env, jclass clazz)
{
	return	((*env)->reserved0 == NULL) &&
			((*env)->reserved1 == NULL) &&
			((*env)->reserved2 == NULL) &&
			((*env)->reserved3 == NULL) &&
			((*env)->GetVersion != NULL);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    getVersion
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_test_java_lang_JNITest_getVersion(JNIEnv *env, jclass clazz)
{
	return (*env)->GetVersion(env);
}

static int readFile(const char *fileName, char **fileBytes)
{
	FILE *file;
	int fileSize;

	file = fopen(fileName, "rb");
	if (!file)
	{
		fprintf(stderr, "can't open file %s\n", fileName);
		return(-1);
	}

	fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	*fileBytes = (char *) malloc(fileSize);

	if (!*fileBytes)
	{
		fprintf(stderr, "jnitest.c: Out of memory error!\n");
		fclose(file);
		return(-1);
	}

	fread(*fileBytes, fileSize, 1, file);
	fclose(file);

	return fileSize;
}


/*
 * Class:     test_java_lang_JNITest
 * Method:    defineClass
 * Signature: (Ljava/lang/String;Ljava/lang/Object;
 */
JNIEXPORT jclass JNICALL Java_test_java_lang_JNITest_defineClass(JNIEnv *env, jclass clazz, jstring name, jobject classloader)
{
	char *nameStr;
	jboolean iscopy;

	nameStr = (char *) (*env)->GetStringUTFChars(env, name, &iscopy);
	if (nameStr == NULL) {
		return NULL; /* OutOfMemoryError */
	}

	char *classBytes;

	int fileSize = readFile(nameStr, &classBytes);

	jclass definedClass = (*env)->DefineClass(env, nameStr, classloader, classBytes, fileSize);

	// Make sure that the buffer we passed is not in use (as specified in JNI 1.6 DefineClass specification)
	memset(classBytes, 0, fileSize);
	free(classBytes);

	(*env)->ReleaseStringUTFChars(env, name, nameStr);

	return definedClass;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testDefineClassThrowsExceptionWithBrokenClass
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_testDefineClassThrowsExceptionWithBrokenClass(JNIEnv *env, jclass clazz)
{
	const char *classBytes = "This represents a broken class";

	jclass definedClass = (*env)->DefineClass(env, "Foo", NULL, classBytes, sizeof(classBytes));

	return 0;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    findClass
 * Signature: (Ljava/lang/String;)Ljava/lang/Class;
 */
JNIEXPORT jclass JNICALL Java_test_java_lang_JNITest_findClass(JNIEnv *env, jclass clazz, jstring name)
{
	char *nameStr;
	jboolean iscopy;

	nameStr = (char *) (*env)->GetStringUTFChars(env, name, &iscopy);
	if (nameStr == NULL) {
		return NULL; /* OutOfMemoryError */
	}

	jclass foundClass = (*env)->FindClass(env, nameStr);

	(*env)->ReleaseStringUTFChars(env, name, nameStr);

	return foundClass;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    passThroughFromAndToReflectedMethod
 * Signature: (Ljava/lang/reflect/Method;)Ljava/lang/reflect/Method;
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_passThroughFromAndToReflectedMethod(JNIEnv *env, jclass clazz, jobject javaMethod)
{
	jmethodID methodID = (*env)->FromReflectedMethod(env, javaMethod);
	return (*env)->ToReflectedMethod(env, clazz, methodID, true);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    passThroughFromAndToReflectedField
 * Signature: (Ljava/lang/reflect/Field;)Ljava/lang/reflect/Field;
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_passThroughFromAndToReflectedField(JNIEnv *env, jclass clazz, jobject javaField)
{
	jfieldID fieldID = (*env)->FromReflectedField(env, javaField);
	return (*env)->ToReflectedField(env, clazz, fieldID, true);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    getSuperclass
 * Signature: (Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_getSuperclass(JNIEnv *env, jclass clazz, jclass childClazz)
{
	return (*env)->GetSuperclass(env, childClazz);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    isAssignableFrom
 * Signature: (Ljava/lang/Class;Ljava/lang/Class;)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_isAssignableFrom(JNIEnv *env, jclass clazz, jclass clazz1, jclass clazz2)
{
	return (*env)->IsAssignableFrom(env, clazz1, clazz2);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    jniThrow
 * Signature: ()I;Ljava/lang/Throwable;
 */
JNIEXPORT jint JNICALL Java_test_java_lang_JNITest_jniThrow(JNIEnv *env, jclass clazz, jthrowable throwable)
{
	return (*env)->Throw(env, throwable);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    jniThrowNew
 * Signature: (Ljava/lang/Class;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_test_java_lang_JNITest_jniThrowNew(JNIEnv *env, jclass clazz, jclass throwable, jstring str)
{
	char *messageStr;
	jboolean iscopy;

	messageStr = (char *) (*env)->GetStringUTFChars(env, str, &iscopy);
	if (messageStr == NULL) {
		return -1; /* OutOfMemoryError */
	}

	return (*env)->ThrowNew(env, throwable, messageStr);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testJniExceptionOccurredAndExceptionClear
 * Signature: (Ljava/lang/Throwable;)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_testJniExceptionOccurredAndExceptionClear(JNIEnv *env, jclass clazz, jclass throwable)
{
	if ((*env)->ExceptionOccurred(env))
		return false;

	(*env)->Throw(env, throwable);

	if (!(*env)->ExceptionOccurred(env))
		return false;

	(*env)->ExceptionClear(env);

	if ((*env)->ExceptionOccurred(env))
		return false;

	return true;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testIsSameObject
 * Signature: (Ljava/lang/Object;Ljava/lang/Object;Ljava/lang/Object;)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_testIsSameObject(JNIEnv *env, jclass clazz, jobject obj, jobject sameObj, jobject differentObj)
{
	if ((*env)->IsSameObject(env, obj, sameObj) && !(*env)->IsSameObject(env, obj, differentObj))
		return true;
	else
		return false;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testAllocObject
 * Signature: (Ljava/lang/Class;)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_testAllocObject(JNIEnv *env, jclass clazz, jclass clazzToAlloc)
{
	if ((*env)->AllocObject(env, clazzToAlloc) != NULL)
		return true;
	else
		return false;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testNewObject
 * Signature: ((Ljava/lang/Class;Ljava/lang/String;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_testNewObject(JNIEnv *env, jclass clazz, jclass clazzToAlloc, jstring constructorSig, jobject args)
{
	char *constructorSigStr;
	jboolean iscopy;

	constructorSigStr = (char *) (*env)->GetStringUTFChars(env, constructorSig, &iscopy);
	if (constructorSigStr == NULL) {
		return NULL; /* OutOfMemoryError */
	}

	jmethodID methodID = (*env)->GetMethodID(env, clazzToAlloc, "<init>", constructorSigStr);
	if (methodID == NULL)
		return NULL;

	if (args != NULL)
		return ((*env)->NewObject(env, clazzToAlloc, methodID, args));
	else
		return ((*env)->NewObject(env, clazzToAlloc, methodID, NULL));
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testNewObjectA
 * Signature: ((Ljava/lang/Class;Ljava/lang/String;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_testNewObjectA(JNIEnv *env, jclass clazz, jclass clazzToAlloc, jstring constructorSig, jobject args)
{
	jvalue jniArgs[1];
	char *constructorSigStr;
	jboolean iscopy;

	constructorSigStr = (char *) (*env)->GetStringUTFChars(env, constructorSig, &iscopy);
	if (constructorSigStr == NULL) {
		return NULL; /* OutOfMemoryError */
	}

	jmethodID methodID = (*env)->GetMethodID(env, clazzToAlloc, "<init>", constructorSigStr);
	if (methodID == NULL)
		return NULL;

	if (args != NULL) {
		jniArgs[0].l = args;
		return ((*env)->NewObjectA(env, clazzToAlloc, methodID, jniArgs));
	}
	else {
		return ((*env)->NewObjectA(env, clazzToAlloc, methodID, NULL));
	}
}

static jobject callNewObjectV(JNIEnv *env, jclass clazzToAlloc, jmethodID methodID, ...)
{
	va_list args;
	jobject jobj;

	va_start(args, methodID);
	jobj = ((*env)->NewObjectV(env, clazzToAlloc, methodID, args));
	va_end(args);

	return jobj;
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testNewObjectV
 * Signature: ((Ljava/lang/Class;Ljava/lang/Class;Ljava/lang/String;Ljava/lang/Object;)Ljava/lang/Object;
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_testNewObjectV(JNIEnv *env, jclass clazz, jclass clazzToAlloc, jstring constructorSig, jobject args)
{
	char *constructorSigStr;
	jboolean iscopy;

	constructorSigStr = (char *) (*env)->GetStringUTFChars(env, constructorSig, &iscopy);
	if (constructorSigStr == NULL) {
		return NULL; /* OutOfMemoryError */
	}

	jmethodID methodID = (*env)->GetMethodID(env, clazzToAlloc, "<init>", constructorSigStr);
	if (methodID == NULL)
		return NULL;

	if (args != NULL)
		return callNewObjectV(env, clazzToAlloc, methodID, args);
	else
		return ((*env)->NewObjectV(env, clazzToAlloc, methodID, NULL));
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testGetObjectClass
 * Signature: (Ljava/lang/Object;)Ljava/lang/Object
 */
JNIEXPORT jobject JNICALL Java_test_java_lang_JNITest_testGetObjectClass(JNIEnv *env, jclass clazz, jobject obj)
{
	return (*env)->GetObjectClass(env, obj);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    isInstanceOf
 * Signature: (Ljava/lang/Class;)Ljava/lang/Object;
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_isInstanceOf(JNIEnv *env, jclass klass, jobject obj, jclass clazz)
{
	return (*env)->IsInstanceOf(env, obj, clazz);
}

/*
 * Class:     test_java_lang_JNITest
 * Method:    testMethodID
 * Signature: (Ljava/lang/Class;Ljava/lang/String;Ljava/lang/String;)Z
 */
JNIEXPORT jboolean JNICALL Java_test_java_lang_JNITest_testMethodID(JNIEnv *env, jclass clazz, jclass classForMethodID, jstring methodName, jstring signature)
{
	char *methodNameStr;
	char *signatureStr;
	jboolean iscopy;

	methodNameStr = (char *) (*env)->GetStringUTFChars(env, methodName, &iscopy);
	if (methodNameStr == NULL) {
		return false; /* OutOfMemoryError */
	}

	signatureStr = (char *) (*env)->GetStringUTFChars(env, signature, &iscopy);
	if (signatureStr == NULL) {
		return false; /* OutOfMemoryError */
	}

	jmethodID methodID = (*env)->GetMethodID(env, classForMethodID, methodNameStr, signatureStr);
	if (methodID == NULL)
		return false;

	(*env)->ReleaseStringUTFChars(env, methodName, methodNameStr);
	(*env)->ReleaseStringUTFChars(env, signature, signatureStr);

	return true;
}
