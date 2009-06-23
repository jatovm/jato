/*
 * Copyright (C) 2006, 2009 Pekka Enberg
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
 * @author Pekka Enberg
 */
public class IntegerArithmeticTest extends TestCase {
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

    public static void testIntegerAdditionLocalSlot() {
        int x = 0x1a;
        int y = 0x0d;
        int result = (0x1f + x) + (0x11 + y);

        assertEquals(0x57, result);
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

    public static void testIntegerSubtractionImmediateLocal() {
        int x = 0x1a;
        int y = 0x0d;
        int result = (0x1f - x) - (0x11 - y);

        assertEquals(1, result);
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

    public static void testIntegerDivisionRegReg() {
        int x = 3;

        try {
            x = 1 / 0;
        } catch (ArithmeticException e) {}

        assertEquals(3, x);
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

    public static void testIntegerRightShiftSignExtends() {
        assertEquals(-1, shr(-2, 1));
    }

    public static int shr(int value, int distance) {
        return value >> distance;
    }

    public static void testIntegerUnsignedRightShift() {
        assertEquals(1, ushr(1, 0));
        assertEquals(0, ushr(1, 1));
        assertEquals(1, ushr(2, 1));
        assertEquals(3, ushr(15, 2));
        assertEquals(0, ushr(Integer.MAX_VALUE, 31));
    }

    public static void testIntegerUnsignedRightShiftDistanceIsMasked() {
        assertEquals(1, ushr(1, 32));
        assertEquals(0, ushr(1, 33));
    }

    public static void testIntegerUnsignedRightShiftZeroExtends() {
        assertEquals(Integer.MAX_VALUE, ushr(-1, 1));
        assertEquals(Integer.MAX_VALUE / 2 + 1, ushr(Integer.MIN_VALUE, 1));
    }

    public static int ushr(int value, int distance) {
        return value >>> distance;
    }

    public static void testIntegerBitwiseInclusiveOr() {
        assertEquals(0, or(0, 0));
        assertEquals(1, or(1, 0));
        assertEquals(1, or(0, 1));
        assertEquals(1, or(1, 1));
    }

    public static int or(int value1, int value2) {
        return value1 | value2;
    }

    public static void testIntegerBitwiseAnd() {
        assertEquals(0, and(0, 0));
        assertEquals(0, and(1, 0));
        assertEquals(0, and(0, 1));
        assertEquals(1, and(1, 1));
    }

    public static int and(int value1, int value2) {
        return value1 & value2;
    }

    public static void testIntegerBitwiseExclusiveOr() {
        assertEquals(0, xor(0, 0));
        assertEquals(1, xor(1, 0));
        assertEquals(1, xor(0, 1));
        assertEquals(0, xor(1, 1));
    }

    public static int xor(int value1, int value2) {
        return value1 ^ value2;
    }

    public static void testIntegerIncrementLocalByConstant() {
        assertEquals(-1, iincByMinusOne(0));
        assertEquals( 1, iincByOne(0));
        assertEquals( 2, iincByOne(1));
        assertEquals( 4, iincByTwo(2));
    }

    public static int iincByMinusOne(int value) {
        return value += -1;
    }

    public static int iincByOne(int value) {
        return value += 1;
    }

    public static int iincByTwo(int value) {
        return value += 2;
    }

    public static void main(String[] args) {
        testIntegerAddition();
        testIntegerAdditionOverflow();
        testIntegerAdditionLocalSlot();
        testIntegerSubtraction();
        testIntegerSubtractionOverflow();
        testIntegerSubtractionImmediateLocal();
        testIntegerMultiplication();
        testIntegerMultiplicationOverflow();
        testIntegerDivision();
        testIntegerDivisionRegReg();
        testIntegerRemainder();
        testIntegerNegation();
        testIntegerNegationOverflow();
        testIntegerLeftShift();
        testIntegerLeftShiftDistanceIsMasked();
        testIntegerRightShift();
        testIntegerRightShiftDistanceIsMasked();
        testIntegerRightShiftSignExtends();
        testIntegerUnsignedRightShift();
        testIntegerUnsignedRightShiftDistanceIsMasked();
        testIntegerUnsignedRightShiftZeroExtends();
        testIntegerBitwiseInclusiveOr();
        testIntegerBitwiseAnd();
        testIntegerBitwiseExclusiveOr();
        testIntegerIncrementLocalByConstant();

        exit();
    }
}
