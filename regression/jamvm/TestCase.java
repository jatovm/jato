/*
 * Copyright (C) 2006-2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

/**
 * @author Pekka Enberg
 */
public class TestCase {
    protected static int retval;

    protected static void assertEquals(int expected, int actual) {
        if (expected != actual) {
            fail(/*"Expected '" + expected + "', but was '" + actual + "'."*/);
        }
    }

    public static void assertEquals(long expected, long actual) {
        if (expected != actual) {
            fail(/*"Expected '" + expected + "', but was '" + actual + "'."*/);
        }
    }

    protected static void assertEquals(Object expected, Object actual) {
        if (expected != actual) {
            fail(/*"Expected '" + expected + "', but was '" + actual + "'."*/);
        }
    }

    protected static void assertNull(Object actual) {
        if (actual != null) {
            fail(/*"Expected null, but was '" + actual + "'."*/);
        }
    }

    protected static void assertNotNull(Object actual) {
        if (actual == null) {
            fail(/*"Expected non-null, but was '" + actual + "'."*/);
        }
    }

    protected static void assertTrue(boolean actual) {
        if (actual == false) {
            fail(/*"Expected true, but was false."*/);
        }
    }

    protected static void assertFalse(boolean actual) {
        if (actual == true) {
            fail(/*"Expected false, but was true."*/);
        }
    }

    protected static void fail(/*String msg*/) {
//        System.out.println(msg);
        retval = 1;
    }
}
