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

    public static void testAthrow() {
        boolean caught = false;
        Exception exception = null;

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

        exit();
    }
};
