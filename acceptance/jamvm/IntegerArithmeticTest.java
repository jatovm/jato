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

    public static int add(int augend, int addend) {
        return augend + addend;
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

    public static int sub(int minuend, int subtrahend) {
        return minuend - subtrahend;
    }

    public static void testIntegerMultiplication() {
        assertEquals( 1, mul(-1, -1));
        assertEquals(-1, mul(-1,  1));
        assertEquals(-1, mul( 1, -1));
        assertEquals( 1, mul( 1,  1));
        assertEquals( 0, mul( 0,  1));
        assertEquals( 0, mul( 1,  0));
        assertEquals( 6, mul( 2,  3));
    }

    public static void testIntegerMultiplicationOverflow() {
        assertEquals(1, mul(Integer.MAX_VALUE, Integer.MAX_VALUE));
        assertEquals(0, mul(Integer.MIN_VALUE, Integer.MIN_VALUE));
        assertEquals(Integer.MIN_VALUE, mul(Integer.MIN_VALUE, Integer.MAX_VALUE));
    }

    public static int mul(int multiplicand, int multiplier) {
        return multiplicand * multiplier;
    }

    public static void testIntegerDivision() {
        assertEquals(-1, div( 1, -1));
        assertEquals(-1, div(-1,  1));
        assertEquals( 1, div(-1, -1));
        assertEquals( 1, div( 1,  1));
        assertEquals( 0, div( 1,  2));
        assertEquals( 1, div( 3,  2));
        assertEquals( 2, div( 2,  1));
        assertEquals( 3, div( 6,  2));
    }

    public static int div(int dividend, int divisor) {
        return dividend / divisor;
    }

    public static void testIntegerRemainder() {
        assertEquals( 1, rem( 3, -2));
        assertEquals(-1, rem(-3,  2));
        assertEquals(-1, rem(-3, -2));
        assertEquals( 1, rem( 3,  2));
        assertEquals( 0, rem( 1,  1));
        assertEquals( 1, rem( 1,  2));
        assertEquals( 2, rem( 5,  3));
    }

    public static int rem(int dividend, int divisor) {
        return dividend % divisor;
    }

    public static void testIntegerNegation() {
        assertEquals(-1, neg( 1));
        assertEquals( 0, neg( 0));
        assertEquals( 1, neg(-1));
    }

    public static void testIntegerNegationOverflow() {
        assertEquals(Integer.MIN_VALUE, neg(Integer.MIN_VALUE));
    }

    public static int neg(int n) {
        return -n;
    }

    public static void testIntegerLeftShift() {
        assertEquals(1, shl(1, 0));
        assertEquals(2, shl(1, 1));
        assertEquals(4, shl(1, 2));
        assertEquals(Integer.MIN_VALUE, shl(1, 31));
    }

    public static void testIntegerLeftShiftDistanceIsMasked() {
        assertEquals(1, shl(1, 32));
        assertEquals(2, shl(1, 33));
    }

    public static int shl(int value, int distance) {
        return value << distance;
    }

    public static void testIntegerRightShift() {
        assertEquals(1, shr(1, 0));
        assertEquals(0, shr(1, 1));
        assertEquals(1, shr(2, 1));
        assertEquals(3, shr(15, 2));
        assertEquals(0, shr(Integer.MAX_VALUE, 31));
    }

    public static void testIntegerRightShiftDistanceIsMasked() {
        assertEquals(1, shr(1, 32));
        assertEquals(0, shr(1, 33));
    }

    public static int shr(int value, int distance) {
        return value >> distance;
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
        testIntegerMultiplication();
        testIntegerMultiplicationOverflow();
        testIntegerDivision();
        testIntegerRemainder();
        testIntegerNegation();
        testIntegerNegationOverflow();
        testIntegerLeftShift();
        testIntegerLeftShiftDistanceIsMasked();
        testIntegerRightShift();
        testIntegerRightShiftDistanceIsMasked();

        Runtime.getRuntime().halt(retval);
    }
}
