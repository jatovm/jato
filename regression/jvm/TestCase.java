/*
 * Copyright (C) 2006-2007, 2009 Pekka Enberg
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

public class TestCase {
    protected static int retval;

    protected static native void exit(int status);

    protected static void assertEquals(int expected, int actual) {
        if (expected != actual) {
            fail(/* "Expected '" + expected + "', but was '" + actual + "'." */);
        }
    }

    public static void assertEquals(long expected, long actual) {
        if (expected != actual) {
            fail(/* "Expected '" + expected + "', but was '" + actual + "'." */);
        }
    }

    protected static void assertEquals(Object expected, Object actual) {
        if (expected != actual) {
            fail(/* "Expected '" + expected + "', but was '" + actual + "'." */);
        }
    }

    protected static void assertNull(Object actual) {
        if (actual != null) {
            fail(/* "Expected null, but was '" + actual + "'." */);
        }
    }

    protected static void assertNotNull(Object actual) {
        if (actual == null) {
            fail(/* "Expected non-null, but was '" + actual + "'." */);
        }
    }

    protected static void assertTrue(boolean actual) {
        if (actual == false) {
            fail(/* "Expected true, but was false." */);
        }
    }

    protected static void assertFalse(boolean actual) {
        if (actual == true) {
            fail(/* "Expected false, but was true." */);
        }
    }

    protected static void fail(/* String msg */) {
        // FIXME:
        // System.out.println(msg);
        retval = 1;
    }
}
