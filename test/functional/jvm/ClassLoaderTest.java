/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */
package jvm;

/**
 * @author Tomasz Grabiec
 */
public class ClassLoaderTest extends TestCase {
    public static void testSystemClassLoaderLoadsAppClass() {
        ClassLoader loader;

        loader = ClassLoaderTest.class.getClassLoader();
        assertNotNull(loader);
        assertEquals(loader, ClassLoader.getSystemClassLoader());
    }

    public static void testLoadClassButDoNotResolveIt() throws Exception {
        StubClassLoader loader = new StubClassLoader();

        Class<?> clazz = loader.loadClass0("jvm.ClassLoaderTest$Clazz", false);
        assertNotNull(clazz);

        assertNotNull(clazz.newInstance());
    }

    public static void testLoadAndResolveClass() throws Exception {
        StubClassLoader loader = new StubClassLoader();

        Class<?> clazz = loader.loadClass0("jvm.ClassLoaderTest$Clazz", true);
        assertNotNull(clazz);

        assertNotNull(clazz.newInstance());
    }

    public static void testResolveClass() throws Exception {
        StubClassLoader loader = new StubClassLoader();

        loader.resolveClass0(Clazz.class);
    }

    public static void main(String[] args) throws Exception {
        testSystemClassLoaderLoadsAppClass();
        testLoadClassButDoNotResolveIt();
        testLoadAndResolveClass();
        testResolveClass();
    }

    private static class StubClassLoader extends ClassLoader {
        public Class<?> loadClass0(String name, boolean resolve) throws ClassNotFoundException {
            return loadClass(name, resolve);
        }

        public final void resolveClass0(Class<?> c) {
            resolveClass(c);
        }
    }

    public static class Clazz {
    }
}
