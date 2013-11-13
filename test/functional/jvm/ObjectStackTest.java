/*
 * Copyright (C) 2009 Pekka Enberg
 * 
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Pekka Enberg
 */
public class ObjectStackTest extends TestCase {
    /*
     * The following method evaluates whether n > 1 is true ("0x01") or false
     * ("0x00") and passes the result to the assertTrue() method. The generated
     * bytecode will have two mutually exclusive basic blocks to handle the
     * evaluation. We're testing here that the object stack state is properly
     * preserved during JIT compilation.
     */
    public static void assertIsGreaterThanOne(int n) {
        assertTrue(n > 1);
    }

    /*
     * The inverse case of the above.
     */
    public static void assertIsNotGreaterThanOne(int n) {
        assertFalse(n > 1);
    }

    public static void testObjectStackWhenBranching() {
        assertIsGreaterThanOne(2);
        assertIsNotGreaterThanOne(1);
    }

    public static void testLoadAndIncrementLocal() {
        int x = 0;

        assertEquals(0, x++);
    }


    public static int classField;

    public static void testLoadAndIncrementClassField() {
        classField = 0;
        assertEquals(0, classField++);
    }

    public int instanceField;

    public static void testLoadAndIncrementInstanceField() {
        ObjectStackTest test = new ObjectStackTest();

        test.instanceField = 0;
        assertEquals(0, test.instanceField++);
    }

    public static void main(String[] args) {
        testObjectStackWhenBranching();
        testLoadAndIncrementLocal();
        testLoadAndIncrementClassField();
        testLoadAndIncrementInstanceField();
    }
}
