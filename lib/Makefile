GLIBJ_ZIP	:= /usr/share/classpath/glibj.zip
JAVAC		:= ecj

JAVA_FILES = jamvm/java/lang/JarLauncher.java \
             java/lang/VMRuntime.java \
             java/lang/VMClassLoader.java \
             java/lang/VMString.java \
             java/lang/VMThread.java \
             java/lang/VMThrowable.java \
             java/lang/reflect/Constructor.java \
             java/lang/reflect/Field.java \
             java/lang/reflect/Method.java \
             java/security/VMAccessController.java \
             gnu/classpath/VMSystemProperties.java \
             gnu/classpath/VMStackWalker.java

inst_classes.zip: classes.zip
	ln -fs $< $@

classes.zip: $(JAVA_FILES)
	mkdir -p classes
	$(JAVAC) -bootclasspath ${GLIBJ_ZIP} -d classes $(JAVA_FILES)
	cd classes && zip -r ../classes.zip .
	rm -rf classes

clean:
	rm -f inst_classes.zip classes.zip
