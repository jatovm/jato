/*
 * Copyright (C) 2006, 2009 Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Pekka Enberg
 */
public class FloatArithmeticTest extends TestCase {
    public static void testFloatAddition() {
        assertEquals(-1.5f, add(0.5f, -2.0f));
        assertEquals( 0.0f, add(1.0f, -1.0f));
        assertEquals( 0.0f, add(0.0f, 0.0f));
        assertEquals( 1.5f, add(0.3f, 1.2f));
        assertEquals( 3.1415f, add(1.14f, 2.0015f));
    }

    public static float add(float augend, float addend) {
        return augend + addend;
    }

    public static void testFloatAdditionLocalSlot() {
        float x = 1.234567f;
        float y = 7.654321f;
        float result = (1.111111f + x) + (0.0f + y);

        assertEquals(9.999999f, result);
    }

    public static void testFloatSubtraction() {
        assertEquals(-1.5f, sub(-2.5f, -1.0f));
        assertEquals( 0.0f, sub(-1.0f, -1.0f));
        assertEquals( 0.0f, sub( 0.0f,  0.0f));
        assertEquals(-1.5f, sub( -0.5f,  1.0f));
        assertEquals(-2.5f, sub( 1,  3.5f));
    }

    public static float sub(float minuend, float subtrahend) {
        return minuend - subtrahend;
    }

    public static void testFloatSubtractionImmediateLocal() {
        float x = 4.5f;
        float y = 3.5f;
        float result = (1.5f - x) - (0.0f - y);

        assertEquals(0.5f, result);
    }

    public static void testFloatMultiplication() {
        assertEquals( 1.0f, mul(-1.0f, -1.0f));
        assertEquals(-3f, mul(-1.5f,  2f));
        assertEquals(-1.0f, mul( 1.0f, -1.0f));
        assertEquals( 12f, mul( -3f,  -4f));
        assertEquals( 0.0f, mul( 0.0f,  1.0f));
        assertEquals( 0.0f, mul( 1.0f,  0.0f));
        assertEquals( 6.4f, mul( 2.0f,  3.2f));
    }

    public static float mul(float multiplicand, float multiplier) {
        return multiplicand * multiplier;
    }

    public static void testFloatDivision() {
        assertEquals(-1.0f, div( 1.0f, -1.0f));
        assertEquals(-1.0f, div(-1.0f,  1.0f));
        assertEquals( 1.0f, div(-1.0f, -1.0f));
        assertEquals( 1.0f, div( 1.0f,  1.0f));
        assertEquals( 1.5f, div( 3.0f,  2.0f));
        assertEquals( 2.0f, div( 2.0f,  1.0f));
        assertEquals( 0.5f, div( 1.0f,  2.0f));
    }

    public static float div(float dividend, float divisor) {
        return dividend / divisor;
    }

    public static void testFloatRemainder() {
        assertEquals(2.0f, rem(6.0f, 4.0f));
        assertEquals(10000.5f, rem(10000.5f, 10001.5f));
        assertEquals(1.0f, rem(10002.5f, 10001.5f));
    }

    public static float rem(float dividend, float divisor) {
        return dividend % divisor;
    }

    public static void testFloatNegation() {
        assertEquals(-1.5f, neg( 1.5f));
        assertEquals( -0.0f, neg( 0.0f));
        assertEquals( 1.3f, neg(-1.3f));
    }

    public static float neg(float n) {
        return -n;
    }

    public static void testFloatComparison() {
        float zero = 0.0f;
        float one = 1.0f;

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
        testFloatAddition();
        testFloatAdditionLocalSlot();
        testFloatSubtraction();
        testFloatSubtractionImmediateLocal();
        testFloatMultiplication();
        testFloatDivision();
        testFloatRemainder();
        testFloatNegation();
        testFloatComparison();
    }
}
