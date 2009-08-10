/*
 * Copyright (C) 2006, 2009 Pekka Enberg
 *                     2009 Tomasz Grabiec
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

    public static double i2d(int val) {
        return (double) val;
    }

    public static int d2i(double val) {
        return (int) val;
    }

    public static double l2d(long val) {
        return (double) val;
    }

    public static long d2l(double val) {
        return (long) val;
    }

    public static void testDoubleIntConversion() {
        assertEquals(2, d2i(2.5));
        assertEquals(-1000, d2i(-1000.0101));
        assertEquals(2.0, i2d(2));
        assertEquals(-3000, i2d(-3000));
    }

    public static void testDoubleLongConversion() {
        assertEquals(2, d2l(2.));
        assertEquals(-1000, d2l(-1000.0101));
        assertEquals(2.0, l2d(2));
        assertEquals(-3000, l2d(-3000));
    }

    private static double zero = 0.0;
    private static double one = 1.0;

    public static void testDoubleComparison() {
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
        testDoubleIntConversion();
        testDoubleLongConversion();
        testDoubleComparison();
    }
}
