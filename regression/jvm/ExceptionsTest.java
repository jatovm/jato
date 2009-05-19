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

class MyException extends Exception {
    static final long serialVersionUID = 0;

    public MyException(String msg) {
        super(msg);
    }
};

class MyException2 extends MyException {
    static final long serialVersionUID = 0;

    public MyException2(String msg) {
        super(msg);
    }
};


/**
 * @author Tomasz Grabiec
 */
public class ExceptionsTest extends TestCase {
    public static void testCatchCompilation() {
        assertEquals(2, onlyTryBlockExecuted());
    }

    public static int onlyTryBlockExecuted() {
        int i = 0;

        try {
            i = 2;
        } catch (Exception e) {
            i = 3;
        } catch (Throwable e) {
            i = 4;
        }

        return i;
    }

    public static void testThrowAndCatchInTheSameMethod() {
        Exception e = new Exception("the horror!");
        boolean catched = false;

        try {
            throw e;
        } catch (Exception _e) {
            assertEquals(e, _e);
            catched = true;
        }

        assertTrue(catched);
    }

    public static void methodThrowingException(int counter) throws Exception {
        if (counter == 0)
            throw new Exception("boom");
        else
            methodThrowingException(counter - 1);
    }

    public static void testUnwinding() {
        boolean catched = false;

        try {
            methodThrowingException(10);
        } catch (Exception e) {
            assertEquals(e.getMessage(), "boom");
            catched = true;
        }

        assertTrue(catched);
    }

    public static void testMultipleCatchBlocks() {
        int section = 0;

        try {
            throw new MyException("boom");
        } catch (MyException2 e) {
            section = 1;
        } catch (MyException e) {
            section = 2;
        } catch (Exception e) {
            section = 3;
        }

        assertEquals(section, 2);
    }

    public static RuntimeException a;
    public static RuntimeException b;

    public static void throwA() {
        a = new RuntimeException();
        throw a;
    }

    public static void throwB() {
        b = new RuntimeException();
        throw b;
    }

    public static void testNestedTryCatch() {
        try {
            throwA();
        } catch (Exception _a) {
            try {
                throwB();
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

    public static void main(String args[]) {
        testCatchCompilation();
        testThrowAndCatchInTheSameMethod();
        testUnwinding();
        testMultipleCatchBlocks();
        testNestedTryCatch();
        testEmptyCatchBlock();

        Runtime.getRuntime().halt(retval);
    }
};
