GLIBJ_ZIP = ${with_classpath_install_dir}/share/classpath/glibj.zip

SUBDIRS = jamvm java gnu
EXTRA_DIST = classes.zip README

noinst_DATA = inst_classes.zip

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
	jikes -bootclasspath ${GLIBJ_ZIP} -d classes $(JAVA_FILES)
	cd classes && zip -r ../classes.zip .
	rm -rf classes

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)
	@use_zip_yes@ cp -p inst_classes.zip $(DESTDIR)$(pkgdatadir)/classes.zip
	@use_zip_no@ unzip -o inst_classes.zip -d $(DESTDIR)$(pkgdatadir)/classes

uninstall-local:
	rm -rf $(DESTDIR)$(pkgdatadir)

DISTCLEANFILES = inst_classes.zip
