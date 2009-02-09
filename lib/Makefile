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
	$(E) "  MKDIR   "
	$(Q) mkdir -p classes
	$(E) "  JAVAC   "
	$(Q) $(JAVAC) -nowarn -bootclasspath $(GLIBJ) -d classes $(VM_CLASSES)
	$(E) "  ZIP     " $@
	$(Q) cd classes && zip -r -q ../classes.zip .

clean:
	$(E) "  CLEAN   "
	$(Q) rm -f classes.zip
	$(Q) rm -rf classes

include ../scripts/build/common.mk
