SUBDIRS = os
DIST_SUBDIRS = os arch

bin_PROGRAMS = jamvm
include_HEADERS = jni.h

jamvm_SOURCES = alloc.c alloc.h cast.c class.c dll.c excep.c execute.c frame.h \
                hash.c hash.h interp.c jam.c jam.h jni.c lock.c lock.h \
                natives.c reflect.c resolve.c sig.h string.c thread.c \
                thread.h utf8.c zip.c zip.h properties.c direct.c interp.h \
                interp-direct.h interp-indirect.h natives.h

LDADD = -lpthread -ldl -lm @extra_libs@ os/@os@/@arch@/libnative.a

AM_CPPFLAGS = -DINSTALL_DIR=\"${prefix}\" -DCLASSPATH_INSTALL_DIR=\"${with_classpath_install_dir}\"

DISTCLEANFILES = arch.h

