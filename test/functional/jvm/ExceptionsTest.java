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
public class ExceptionsTest extends TestCase {
    public static void testTryBlockDoesNotThrowAnything() {
        boolean caught;
        try {
            caught = false;
        } catch (Exception e) {
            caught = true;
        }

        assertFalse(caught);
    }

    public static void testThrowAndCatchInTheSameMethod() {
        Exception exception = null;
        boolean caught;
        try {
            caught = false;
            throw exception = new Exception();
        } catch (Exception e) {
            assertEquals(exception, e);
            caught = true;
        }
        assertTrue(caught);
    }

    public static void testUnwinding() {
        String s = "unwind";
        boolean caught = false;
        try {
            recurseTimesThenThrow(10, s);
        } catch (Exception e) {
            assertEquals(e.getMessage(), s);
            caught = true;
        }
        assertTrue(caught);
    }

    public static void recurseTimesThenThrow(int counter, String s) {
        if (counter == 0)
            throw new RuntimeException(s);

        recurseTimesThenThrow(counter - 1, s);
    }

    public static void testMultipleCatchBlocks() {
        int section = 0;

        try {
            throw new MyException();
        } catch (MyException2 e) {
            section = 1;
        } catch (MyException e) {
            section = 2;
        } catch (Exception e) {
            section = 3;
        }

        assertEquals(section, 2);
    }

    private static class MyException extends Exception {
        static final long serialVersionUID = 0;
    };

    private static class MyException2 extends MyException {
        static final long serialVersionUID = 0;
    };

    public static void throwException(Exception e) throws Exception {
        throw e;
    }

    public static void testNestedTryCatch() {
        Exception a = new RuntimeException();
        Exception b = new RuntimeException();

        try {
            throwException(a);
        } catch (Exception _a) {
            try {
                throwException(b);
            } catch (Exception _b) {
                assertEquals(b, _b);
            }
            assertEquals(a, _a);
        }
    }

    public static void testEmptyCatchBlock() {
        try {
            throw new Exception();
        } catch (Exception e) {}
    }

    private static Exception getNullException() {
        return null;
    }

    public static void testAthrow() {
        boolean caught = false;
        Exception exception = getNullException();

        try {
            throw exception;
        } catch (NullPointerException e) {
            caught = true;
        } catch (Exception e) {
        }

        assertTrue(caught);
    }

    public static void main(String args[]) {
        testTryBlockDoesNotThrowAnything();
        testThrowAndCatchInTheSameMethod();
        testUnwinding();
        testMultipleCatchBlocks();
        testNestedTryCatch();
        testEmptyCatchBlock();
        testAthrow();
    }
}
