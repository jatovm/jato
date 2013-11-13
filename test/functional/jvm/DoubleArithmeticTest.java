/*
 * Copyright (C) 2006, 2009 Pekka Enberg
 *                     2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Tomasz Grabiec
 */
public class DoubleArithmeticTest extends TestCase {
    public static void testDoubleAddition() {
        assertEquals(-1.5, add(0.5, -2.0));
        assertEquals( 0.0, add(1.0, -1.0));
        assertEquals( 0.0, add(0.0, 0.0));
        assertEquals( 1.5, add(0.3, 1.2));
        assertEquals( 3.14159, add(1.141, 2.00059));
    }

    public static double add(double augend, double addend) {
        return augend + addend;
    }

    public static void testDoubleAdditionLocalSlot() {
        double x = 1.234567;
        double y = 7.654321;
        double result = (1.111111 + x) + (0.0 + y);

        assertEquals(9.999999, result);
    }

    public static void testDoubleSubtraction() {
        assertEquals(-1.5, sub(-2.5, -1.0));
        assertEquals( 0.0, sub(-1.0, -1.0));
        assertEquals( 0.0, sub( 0.0,  0.0));
        assertEquals(-1.5, sub( -0.5,  1.0));
        assertEquals(-2.5, sub( 1,  3.5));
    }

    public static double sub(double minuend, double subtrahend) {
        return minuend - subtrahend;
    }

    public static void testDoubleSubtractionImmediateLocal() {
        double x = 4.5;
        double y = 3.5;
        double result = (1.5 - x) - (0.0 - y);

        assertEquals(0.5, result);
    }

    public static void testDoubleMultiplication() {
        assertEquals( 1.0, mul(-1.0, -1.0));
        assertEquals(-3, mul(-1.5,  2));
        assertEquals(-1.0, mul( 1.0, -1.0));
        assertEquals( 12, mul( -3,  -4));
        assertEquals( 0.0, mul( 0.0,  1.0));
        assertEquals( 0.0, mul( 1.0,  0.0));
        assertEquals( 6.4, mul( 2.0,  3.2));
    }

    public static double mul(double multiplicand, double multiplier) {
        return multiplicand * multiplier;
    }

    public static void testDoubleDivision() {
        assertEquals(-1.0, div( 1.0, -1.0));
        assertEquals(-1.0, div(-1.0,  1.0));
        assertEquals( 1.0, div(-1.0, -1.0));
        assertEquals( 1.0, div( 1.0,  1.0));
        assertEquals( 1.5, div( 3.0,  2.0));
        assertEquals( 2.0, div( 2.0,  1.0));
        assertEquals( 0.5, div( 1.0,  2.0));
    }

    public static double div(double dividend, double divisor) {
        return dividend / divisor;
    }

    public static void testDoubleRemainder() {
        assertEquals(2.0, rem(6.0, 4.0));
        assertEquals(10000.5, rem(10000.5, 10001.5));
        assertEquals(1.0, rem(10002.5, 10001.5));
    }

    public static double rem(double dividend, double divisor) {
        return dividend % divisor;
    }

    public static void testDoubleNegation() {
        assertEquals(-1.5, neg( 1.5));
        assertEquals( -0.0, neg( 0.0));
        assertEquals( 1.3, neg(-1.3));
    }

    public static double neg(double n) {
        return -n;
    }

    public static void testDoubleComparison() {
        double zero = 0.0;
        double one = 1.0;

        assertTrue(zero < one);
        assertFalse(one < zero);
        assertFalse(one < one);

        assertTrue(zero <= one);
        assertTrue(one <= one);

        assertFalse(zero > one);
        assertTrue(one > zero);
        assertFalse(one > one);

        assertTrue(one >= zero);
        assertTrue(one >= one);
    }


    public static void main(String[] args) {
        testDoubleAddition();
        testDoubleAdditionLocalSlot();
        testDoubleSubtraction();
        testDoubleSubtractionImmediateLocal();
        testDoubleMultiplication();
        testDoubleDivision();
        testDoubleRemainder();
        testDoubleNegation();
        testDoubleComparison();
    }
}
