VM_CLASSES = \
	jamvm/java/lang/JarLauncher.java	\
	java/lang/VMRuntime.java		\
	java/lang/VMClassLoader.java		\
	java/lang/VMString.java			\
	java/lang/VMThread.java			\
	java/lang/VMThrowable.java		\
	java/lang/reflect/Constructor.java	\
	java/lang/reflect/Field.java		\
	java/lang/reflect/Method.java		\
	java/security/VMAccessController.java	\
	gnu/classpath/VMSystemProperties.java	\
	gnu/classpath/VMStackWalker.java

classes.zip: $(VM_CLASSES)
	mkdir -p classes
	$(JAVAC) -bootclasspath $(GLIBJ) -d classes $(VM_CLASSES)
	cd classes && zip -r ../classes.zip .
	rm -rf classes

clean:
	rm -f classes.zip
