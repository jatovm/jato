/*
 * Copyright (C) 2008  Saeed Siam
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Saeed Siam
 */
public class SynchronizationTest extends TestCase {
    public static void testMonitorEnterAndExit() {
        Object obj = new Object();
        synchronized (obj) {
            assertNotNull(obj);
        }
    }

    public static synchronized int staticSynchronizedMethod(int x) {
        return x;
    }

    public synchronized int synchronizedMethod(int x) {
        return x;
    }

    public static synchronized void staticSynchronizedExceptingMethod() {
        throw new RuntimeException();
    }

    public synchronized void synchronizedExceptingMethod() {
        throw new RuntimeException();
    }

    public static void testSynchronizedExceptingMethod() {
        SynchronizationTest test = new SynchronizationTest();
        boolean caught = false;

        try {
            test.synchronizedExceptingMethod();
        } catch (RuntimeException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testSynchronizedMethod() {
        SynchronizationTest test = new SynchronizationTest();

        assertEquals(3, test.synchronizedMethod(3));
    }

    public static void testStaticSynchronizedExceptingMethod() {
        boolean caught = false;

        try {
            staticSynchronizedExceptingMethod();
        } catch (RuntimeException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testStaticSynchronizedMethod() {
        assertEquals(3, staticSynchronizedMethod(3));
    }


    public static void main(String[] args) {
        testMonitorEnterAndExit();
        testStaticSynchronizedMethod();
        testSynchronizedMethod();
        testStaticSynchronizedExceptingMethod();
        testSynchronizedExceptingMethod();
    }
}
