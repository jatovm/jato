srcdir=.
GLIBJ_ZIP = /usr/local/classpath/share/classpath/glibj.zip
JAVAC=jikes

JAVA_FILES = $(srcdir)/jamvm/java/lang/JarLauncher.java \
             $(srcdir)/java/lang/VMRuntime.java \
             $(srcdir)/java/lang/VMClassLoader.java \
             $(srcdir)/java/lang/VMString.java \
             $(srcdir)/java/lang/VMThread.java \
             $(srcdir)/java/lang/VMThrowable.java \
             $(srcdir)/java/lang/reflect/Constructor.java \
             $(srcdir)/java/lang/reflect/Field.java \
             $(srcdir)/java/lang/reflect/Method.java \
             $(srcdir)/java/security/VMAccessController.java \
             $(srcdir)/gnu/classpath/VMSystemProperties.java \
             $(srcdir)/gnu/classpath/VMStackWalker.java


inst_classes.zip: classes.zip
	ln -fs $< $@

classes.zip: $(JAVA_FILES)
	-mkdir classes
	$(JAVAC) -bootclasspath ${GLIBJ_ZIP} -d classes $(JAVA_FILES)
	cd classes && zip -r ../classes.zip .
	rm -rf classes

clean:
	rm -f inst_classes.zip classes.zip
