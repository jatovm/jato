PRELOAD_FIELD(vm_java_lang_Class,			vmdata,		"Ljava/lang/Object;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_String,			offset,		"I",				PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_String,			count,		"I",				PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_String,			value,		"[C",				PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Throwable,			detailMessage,	"Ljava/lang/String;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_VMThrowable,			vmdata,		"Ljava/lang/Object;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			daemon,		"Z",				PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			group,		"Ljava/lang/ThreadGroup;",	PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			name,		"Ljava/lang/String;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			priority,	"I",				PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			contextClassLoader, "Ljava/lang/ClassLoader;",	PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			contextClassLoaderIsSystemClassLoader, "Z",	PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_Thread,			vmThread,	"Ljava/lang/VMThread;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_VMThread,			thread,		"Ljava/lang/Thread;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_VMThread,			vmdata,		"Ljava/lang/Object;",		PRELOAD_MANDATORY)

PRELOAD_FIELD(vm_java_lang_reflect_Constructor,		clazz,		"Ljava/lang/Class;", 		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_Constructor,		slot,		"I",				PRELOAD_OPTIONAL)
/* Classpath 0.98 */
PRELOAD_FIELD(vm_java_lang_reflect_Constructor,		cons,		"Ljava/lang/reflect/VMConstructor;", PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMConstructor,	clazz,		"Ljava/lang/Class;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMConstructor,	slot,		"I", 				PRELOAD_OPTIONAL)

PRELOAD_FIELD(vm_java_lang_reflect_Field,		declaringClass, "Ljava/lang/Class;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_Field,		slot,		"I",				PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_Field,		name,		"Ljava/lang/String;",		PRELOAD_OPTIONAL)
/* Classpath 0.98 */
PRELOAD_FIELD(vm_java_lang_reflect_Field,		f,		"Ljava/lang/reflect/VMField;",	PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMField,		clazz,		"Ljava/lang/Class;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMField,		slot,		"I",				PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMField,		name,		"Ljava/lang/String;",		PRELOAD_OPTIONAL)

PRELOAD_FIELD(vm_java_lang_reflect_Method,	 	declaringClass, "Ljava/lang/Class;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_Method, 		name,		"Ljava/lang/String;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_Method,		slot,		"I",				PRELOAD_OPTIONAL)
/* Classpath 0.98 */
PRELOAD_FIELD(vm_java_lang_reflect_Method,		m,		"Ljava/lang/reflect/VMMethod;", PRELOAD_OPTIONAL)

PRELOAD_FIELD(vm_java_lang_reflect_VMMethod,		clazz,		"Ljava/lang/Class;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMMethod,		m,		"Ljava/lang/reflect/Method;",	PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMMethod,		name,		"Ljava/lang/String;",		PRELOAD_OPTIONAL)
PRELOAD_FIELD(vm_java_lang_reflect_VMMethod,		slot,		"I",				PRELOAD_OPTIONAL)

PRELOAD_FIELD(vm_java_lang_ref_Reference,		referent,	"Ljava/lang/Object;",		PRELOAD_MANDATORY)
PRELOAD_FIELD(vm_java_lang_ref_Reference,		lock,		"Ljava/lang/Object;",		PRELOAD_MANDATORY)

PRELOAD_FIELD(vm_java_nio_Buffer,			address,	"Lgnu/classpath/Pointer;",	PRELOAD_MANDATORY)

#ifdef CONFIG_32_BIT
PRELOAD_FIELD(vm_gnu_classpath_PointerNN,		data,		"I",				PRELOAD_MANDATORY)
#else
PRELOAD_FIELD(vm_gnu_classpath_PointerNN,		data,		"J",				PRELOAD_MANDATORY)
#endif
