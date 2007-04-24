/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007
 * Robert Lougher <rob@lougher.org.uk>.
 *
 * This file is part of JamVM.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "jam.h"

/* If we have endian.h include it.  Otherwise, include sys/param.h
   if we have it. If the BYTE_ORDER macro is still undefined, we
   fall-back, and work out the endianness ourselves at runtime --
   this always works.
*/
#ifdef HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

static Property *commandline_props;
static int commandline_props_count;

void initialiseProperties(InitArgs *args) {
    commandline_props = args->commandline_props;
    commandline_props_count = args->props_count;
}

char *getCommandLineProperty(char *key) {
    int i;

    for(i = 0; i < commandline_props_count; i++)
        if(strcmp(commandline_props[i].key, key) == 0)
            return commandline_props[i].value;

    return NULL;
}

void setProperty(Object *properties, char *key, char *value) {
    Object *k = Cstr2String(key);
    Object *v = Cstr2String(value ? value : "?");

    MethodBlock *mb = lookupMethod(properties->class, "put",
                           "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
    executeMethod(properties, mb, k, v);
}

void addCommandLineProperties(Object *properties) {
    if(commandline_props_count) {
        int i;

        for(i = 0; i < commandline_props_count; i++) {
            setProperty(properties, commandline_props[i].key, commandline_props[i].value);
            free(commandline_props[i].key);
        }

        commandline_props_count = 0;
        free(commandline_props);
    }
}

void setLocaleProperties(Object *properties) {
#if defined(HAVE_SETLOCALE) && defined(HAVE_LC_MESSAGES)
    char *locale;

    setlocale(LC_ALL, "");
    if((locale = setlocale(LC_MESSAGES, "")) != NULL) {
        int len = strlen(locale);

        /* Check if the returned string is in the expected format,
           e.g. de, or en_GB */
        if(len == 2 || (len > 4 && locale[2] == '_')) {
            char code[3];

            code[0] = locale[0];
            code[1] = locale[1];
            code[2] = '\0';
            setProperty(properties, "user.language", code);

            /* Set the region -- the bit after the "_" */
            if(len > 4) {
                code[0] = locale[3];
                code[1] = locale[4];
                setProperty(properties, "user.region", code);
            }
        }
    }
#endif
}

void setEndianProperty(Object *properties) {
#ifdef BYTE_ORDER
#if BYTE_ORDER == BIG_ENDIAN
    setProperty(properties, "gnu.cpu.endian", "big");
#else
    setProperty(properties, "gnu.cpu.endian", "little");
#endif
#else
    /* No byte-order macro -- work it out ourselves at runtime */
    union {
        int i;
        char c[sizeof(int)];
    } u;

    u.i = 1;
    setProperty(properties, "gnu.cpu.endian",
                u.c[sizeof(int)-1] == 1 ? "big" : "little");
#endif
}

void setUserDirProperty(Object *properties) {
    char *cwd = getcwd(NULL, 0);

    setProperty(properties, "user.dir", cwd);

    if(cwd != NULL)
        free(cwd);
}

void setOSProperties(Object *properties) {
    struct utsname info;

    uname(&info);
    setProperty(properties, "os.arch", OS_ARCH);
    setProperty(properties, "os.name", info.sysname);
    setProperty(properties, "os.version", info.release);
}

char *getJavaHome() {
    char *env = getenv("JAVA_HOME");
    return env ? env : INSTALL_DIR;
}

void addDefaultProperties(Object *properties) {
    setProperty(properties, "java.vm.name", "JamVM");
    setProperty(properties, "java.vm.version", VERSION);
    setProperty(properties, "java.runtime.version", VERSION);
    setProperty(properties, "java.vm.vendor", "Robert Lougher");
    setProperty(properties, "java.vm.vendor.url", "http://jamvm.sourceforge.net");
    setProperty(properties, "java.version", JAVA_COMPAT_VERSION);
    setProperty(properties, "java.vendor", "GNU Classpath");
    setProperty(properties, "java.vendor.url", "http://www.classpath.org");
    setProperty(properties, "java.home", getJavaHome());
    setProperty(properties, "java.specification.version", "1.4");
    setProperty(properties, "java.specification.vendor", "Sun Microsystems, Inc.");
    setProperty(properties, "java.specification.name", "Java Platform API Specification");
    setProperty(properties, "java.vm.specification.version", "1.0");
    setProperty(properties, "java.vm.specification.vendor", "Sun Microsystems, Inc.");
    setProperty(properties, "java.vm.specification.name", "Java Virtual Machine Specification");
    setProperty(properties, "java.class.version", "48.0");
    setProperty(properties, "java.class.path", getClassPath());
    setProperty(properties, "sun.boot.class.path", getBootClassPath());
    setProperty(properties, "java.boot.class.path", getBootClassPath());
    setProperty(properties, "gnu.classpath.boot.library.path", getBootDllPath());
    setProperty(properties, "java.library.path", getDllPath());
    setProperty(properties, "java.io.tmpdir", "/tmp");
    setProperty(properties, "java.compiler", "");
    setProperty(properties, "java.ext.dirs", "");
    setProperty(properties, "file.separator", "/");
    setProperty(properties, "path.separator", ":");
    setProperty(properties, "line.separator", "\n");
    setProperty(properties, "user.name", getenv("USER"));
    setProperty(properties, "user.home", getenv("HOME"));

    setOSProperties(properties);
    setEndianProperty(properties);
    setUserDirProperty(properties);
    setLocaleProperties(properties);
}

