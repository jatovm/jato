/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
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
