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
public class FinallyTest extends TestCase {
    public static int methodWithSingleFinallyBlock() {
	    while (true) {
		    try {
			    return 1;
		    } finally {
			    if (true)
				    break;
		    }
	    }

	    return 0;
    }

    public static void testSingleFinallyBlock() {
        assertEquals(0, methodWithSingleFinallyBlock());
    }

    public static int methodWithNestedFinallyBlocks() {
        boolean b = true;

        try {
            return 1;
        } finally {
            while (b) {
                try {
                    return 2;
                } finally {
                    if (b) {
                        break;
                    }
                }
            }

            if (true)
                return 0;
        }
    }

    public static void testNestedFinallyBlocks() {
        assertEquals(0, methodWithNestedFinallyBlocks());
    }

    private static Exception getNullException() {
        return null;
    }

    public static void testLineNumberTableAfterInlining() {
        Exception e = getNullException();
        boolean b = true;

        try {
            return;
        } finally {
            while (b) {
                try {
                    return;
                } finally {
                    if (b) {
                        e = new Exception();
                        break;
                    }
                }
            }

            assertEquals(71, e.getStackTrace()[0].getLineNumber());
        }
    }

    public static void testExceptionTableAfterInlining() {
        boolean caught = false;
        boolean caughtInFinally = false;
        RuntimeException r1 = new RuntimeException();
        RuntimeException r2 = new RuntimeException();

        try {
            throw r1;
        } catch (Exception e) {
            assertEquals(r1, e);
            caught = true;
        } finally {
            try {
                throw r2;
            } catch (Exception e) {
                assertEquals(r2, e);
                caughtInFinally = true;
            }
        }

        assertTrue(caughtInFinally);
        assertTrue(caught);
    }

    public static void testTableswitchInlining() {
        int x = 0;

        try {
            x = 1;
        } finally {
            switch (x) {
            case 0:  x = 2; break;
            case 1:  x = 3; break;
            default: x = 4; break;
            }
        }

        assertEquals(3, x);
    }

    public static void testLookupswitchInlining() {
        int x = 0;

        try {
            x = 1000;
        } finally {
            switch (x) {
            case -100: x = 2; break;
            case 1000: x = 3; break;
            default:   x = 4; break;
            }
        }

        assertEquals(3, x);
    }

    public static void main(String args[]) {
        testSingleFinallyBlock();
        testNestedFinallyBlocks();
        testLineNumberTableAfterInlining();
        testExceptionTableAfterInlining();
        testTableswitchInlining();
        testLookupswitchInlining();
    }
}
