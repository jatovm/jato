dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/jam.c)
AM_INIT_AUTOMAKE(jamvm, 1.4.2)
AC_CONFIG_HEADERS([src/config.h])

AC_ARG_ENABLE(tracegc,
    [  --enable-tracegc         add gc tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACEGC],1,[defined if gc tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(tracealloc,
    [  --enable-tracealloc      add object allocation tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACEALLOC],1,[defined if object allocation tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(tracefnlz,
    [  --enable-tracefnlz       add object finalisation tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACEFNLZ],1,[defined if object finalisation tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(tracedll,
    [  --enable-tracedll        add library loading/lookup tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACEDLL],1,[defined if library loading/unloading tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(tracelock,
    [  --enable-tracelock       add object locking tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACELOCK],1,[defined if object locking tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(tracethread,
    [  --enable-tracethread     add thread creation tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACETHREAD],1,[defined if thread creation tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(trace,
    [  --enable-trace           add all tracing (for debugging)],
    [if test "$enableval" != no; then
        AC_DEFINE([TRACEGC],1,[defined if gc tracing enabled for debugging])
        AC_DEFINE([TRACEALLOC],1,[defined if object allocation tracing enabled for debugging])
        AC_DEFINE([TRACEFNLZ],1,[defined if object finalisation tracing enabled for debugging])
        AC_DEFINE([TRACEDLL],1,[defined if library loading/unloading tracing enabled for debugging])
        AC_DEFINE([TRACELOCK],1,[defined if object locking tracing enabled for debugging])
        AC_DEFINE([TRACETHREAD],1,[defined if thread creation tracing enabled for debugging])
    fi],)

AC_ARG_ENABLE(zip,
    [  --disable-zip            turn-off zip support in the bootstrap loader],,)

AC_CANONICAL_HOST

case "$host" in
i[[456]]86-*-linux*) host_cpu=i386 host_os=linux ;;
i[[456]]86-*-kfreebsd*) host_cpu=i386 host_os=linux ;;
i386-*-openbsd*) host_os=linux libdl_needed=no ;;
i386-*-freebsd*) host_os=linux libdl_needed=no ;;
x86_64-*-linux*) host_os=linux ;;
amd64-*-openbsd*) host_os=linux libdl_needed=no ;;
amd64-*-freebsd*) host_os=linux libdl_needed=no ;;
arm*-*-linux*) host_cpu=arm host_os=linux ;;
arm*-*-openbsd*) host_cpu=arm host_os=linux libdl_needed=no ;;
arm*-*-freebsd*) host_cpu=arm host_os=linux libdl_needed=no ;;
powerpc*-*-linux*) host_cpu=powerpc host_os=linux ;;
powerpc*-*-openbsd*) host_cpu=powerpc host_os=linux libdl_needed=no ;;
powerpc*-*-freebsd*) host_cpu=powerpc host_os=linux libdl_needed=no ;;
powerpc-*-darwin*) host_os=darwin ;;
*) AC_MSG_ERROR($host not supported) ;;
esac

arch=$host_cpu
os=$host_os
AC_SUBST(arch)
AC_SUBST(os)

AC_ARG_ENABLE(int-threading,
    [  --enable-int-threading   enable threaded version of the interpreter (default enabled)],,)

AC_ARG_ENABLE(int-direct,
    [  --enable-int-direct      enable direct threaded version of the interpreter (default enabled)],,)

AC_ARG_ENABLE(int-caching,
    [  --enable-int-caching     enable stack-caching version of the interpreter
                                  (default powerpc,arm enabled : i386 disabled)],,
    [if test "$host_cpu" == i386; then
         enable_int_caching=no
     fi])

AC_ARG_ENABLE(int-prefetch,
    [  --enable-int-prefetch    enable prefetching version of the interpreter
                                  (default powerpc enabled : i386,arm disabled)],,
    [if test "$host_cpu" != powerpc; then
         enable_int_prefetch=no
     fi])

if test "$enable_int_threading" != no; then
    AC_DEFINE([THREADED],1,[interpreter threaded])

    if test "$enable_int_direct" != no; then
        AC_DEFINE([DIRECT],1,[interpreter direct])
    fi

    if test "$enable_int_caching" != no; then
        AC_DEFINE([USE_CACHE],1,[interpreter uses caching])
    fi

    if test "$enable_int_prefetch" != no; then
        AC_DEFINE([PREFETCH],1,[interpreter uses prefetching])
    fi
fi

AC_ARG_ENABLE(ffi,
    [  --enable-ffi             use libffi to call native methods],,
    [if test "$host_cpu" != x86_64; then
        enable_ffi=no
     fi])

AC_ARG_WITH(classpath-install-dir,
    [  --with-classpath-install-dir=<dir>       installation directory of GNU classpath
                                (default /usr/local/classpath)],,
    [with_classpath_install_dir=/usr/local/classpath])

AC_SUBST(with_classpath_install_dir)

dnl Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

AM_PROG_AS

AC_CHECK_PROGS(JAVAC, ecj jikes "gcj -C" javac)

dnl Checks for libraries.

AC_CHECK_LIB(pthread,pthread_self,,AC_MSG_ERROR(libpthread is missing))
AC_CHECK_LIB(m,fmod,,AC_MSG_ERROR(libm is missing))

if test "$libdl_needed" != no; then
    AC_CHECK_LIB(dl,dlopen,,AC_MSG_ERROR(libdl is missing))
fi

AC_CHECK_LIB(z,inflate,,
    [if test "$enable_zip" != no; then
        AC_MSG_WARN(cannot find libz -- zip/jar support in bootclasspath is disabled)
        enable_zip=no
    fi])

AC_CHECK_LIB(ffi,ffi_call,,
    [if test "$enable_ffi" != no; then
        AC_MSG_ERROR(cannot find libffi)
    fi])

dnl Checks for header files.
AC_HEADER_STDC

AC_CHECK_HEADER(zlib.h,,
    [if test "$enable_zip" != no; then
        AC_MSG_WARN(cannot find zlib.h -- zip/jar support in bootclasspath is disabled)
        enable_zip=no
    fi])

AC_CHECK_HEADER(ffi.h,,
    [if test "$enable_ffi" != no; then
        AC_MSG_ERROR(cannot find ffi.h)
    fi])

if test "$enable_zip" != no; then
    AC_DEFINE([USE_ZIP],1,[use zip])
    use_zip_yes=
    use_zip_no='#'
else
    use_zip_yes='#'
    use_zip_no=
fi

AC_SUBST(use_zip_yes)
AC_SUBST(use_zip_no)

if test "$enable_ffi" != no; then
    AC_DEFINE([USE_FFI],1,[use FFI])
fi

AC_CHECK_HEADERS(sys/time.h unistd.h endian.h sys/param.h locale.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_ALLOCA
AC_FUNC_MMAP
AC_CHECK_FUNCS(gettimeofday strtol setlocale)
AM_LC_MESSAGES

AM_MAINTAINER_MODE

AC_CONFIG_LINKS(src/arch.h:src/arch/$arch.h)

AC_CONFIG_FILES(
    Makefile \
    src/Makefile \
    src/arch/Makefile \
    src/os/Makefile \
    src/os/linux/Makefile \
    src/os/darwin/Makefile \
    src/os/linux/powerpc/Makefile \
    src/os/linux/arm/Makefile \
    src/os/linux/i386/Makefile \
    src/os/linux/x86_64/Makefile \
    src/os/darwin/powerpc/Makefile \
    lib/Makefile \
    lib/java/Makefile \
    lib/java/lang/Makefile \
    lib/jamvm/Makefile \
    lib/jamvm/java/Makefile \
    lib/jamvm/java/lang/Makefile \
    lib/java/lang/reflect/Makefile \
    lib/java/security/Makefile \
    lib/gnu/Makefile \
    lib/gnu/classpath/Makefile)

AC_OUTPUT

