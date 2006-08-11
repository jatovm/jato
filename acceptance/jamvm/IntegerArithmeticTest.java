/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

/**
 * @author Pekka Enberg
 */
public class IntegerArithmeticTest {
    private static int retval;

    public static void testIntegerAddition() {
        assertEquals(-1, add(0, -1));
        assertEquals( 0, add(1, -1));
        assertEquals( 0, add(0, 0));
        assertEquals( 1, add(0, 1));
        assertEquals( 3, add(1, 2));
    }

    public static void testIntegerAdditionOverflow() {
        assertEquals(Integer.MAX_VALUE, add(0, Integer.MAX_VALUE));
        assertEquals(Integer.MIN_VALUE, add(1, Integer.MAX_VALUE));
    }

    public static int add(int a, int b) {
        return a + b;
    }

    public static void testIntegerSubtraction() {
        assertEquals(-1, sub(-2, -1));
        assertEquals( 0, sub(-1, -1));
        assertEquals( 0, sub( 0,  0));
        assertEquals(-1, sub( 0,  1));
        assertEquals(-2, sub( 1,  3));
    }

    public static void testIntegerSubtractionOverflow() {
        assertEquals(Integer.MIN_VALUE, sub(Integer.MIN_VALUE, 0));
        assertEquals(Integer.MAX_VALUE, sub(Integer.MIN_VALUE, 1));
    }

    public static int sub(int a, int b) {
        return a - b;
    }

    private static void assertEquals(int expected, int actual) {
        if (expected != actual) {
            fail(/*"Expected '" + expected + "', but was '" + actual + "'."*/);
        }
    }

    private static void fail(/*String msg*/) {
        //System.out.println(msg);
        retval = 1;
    }

    public static void main(String[] args) {
        testIntegerAddition();
        testIntegerAdditionOverflow();
	testIntegerSubtraction();
	testIntegerSubtractionOverflow();

        Runtime.getRuntime().halt(retval);
    }
}
