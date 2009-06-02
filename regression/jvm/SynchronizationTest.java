/*
 * Copyright (C) 2008  Saeed Siam
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

        exit();
    }
}
