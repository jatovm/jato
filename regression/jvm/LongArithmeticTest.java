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
public class LongArithmeticTest extends TestCase {
    public static void testLongAddition() {
        assertEquals(-1, add(0, -1));
        assertEquals( 0, add(1, -1));
        assertEquals( 0, add(0, 0));
        assertEquals( 1, add(0, 1));
        assertEquals( 0x200000001L, add(0x100000000L, 0x100000001L));
    }

    public static void testLongAdditionOverflow() {
        assertEquals(Long.MAX_VALUE, add(0, Long.MAX_VALUE));
        assertEquals(Long.MIN_VALUE, add(1, Long.MAX_VALUE));
    }

    public static long add(long augend, long addend) {
        return augend + addend;
    }

    public static void testLongAdditionLocalSlot() {
        long x = 0x1a;
        long y = 0x0d;
        long result = (0x1f + x) + (0x11 + y);

        assertEquals(0x57, result);
    }

    public static void testLongSubtraction() {
        assertEquals(-1, sub(-2, -1));
        assertEquals( 0, sub(-1, -1));
        assertEquals( 0, sub( 0,  0));
        assertEquals(-1, sub( 0,  1));
        assertEquals( 0x200000000L, sub( 0x200000001L, 1L));
    }

    public static void testLongSubtractionOverflow() {
        assertEquals(Long.MIN_VALUE, sub(Long.MIN_VALUE, 0));
        assertEquals(Long.MAX_VALUE, sub(Long.MIN_VALUE, 1));
    }

    public static long sub(long minuend, long subtrahend) {
        return minuend - subtrahend;
    }

    public static void testLongSubtractionImmediateLocal() {
        long x = 0x1a;
        long y = 0x0d;
        long result = (0x1f - x) - (0x11 - y);

        assertEquals(1, result);
    }

    public static void testLongMultiplication() {
        assertEquals( 1, mul(-1, -1));
        assertEquals(-1, mul(-1,  1));
        assertEquals(-1, mul( 1, -1));
        assertEquals( 1, mul( 1,  1));
        assertEquals( 0, mul( 0,  1));
        assertEquals( 0, mul( 1,  0));
        assertEquals( 6, mul( 2,  3));
        assertEquals(-0x200000000L, mul( 0x100000000L, -2L));
        assertEquals(-0x400000000L, mul(-0x200000000L,  2L));
    }

    public static void testLongMultiplicationOverflow() {
        assertEquals(1, mul(Long.MAX_VALUE, Long.MAX_VALUE));
        assertEquals(0, mul(Long.MIN_VALUE, Long.MIN_VALUE));
        assertEquals(Long.MIN_VALUE, mul(Long.MIN_VALUE, Long.MAX_VALUE));
    }

    public static long mul(long multiplicand, long multiplier) {
        return multiplicand * multiplier;
    }

    public static void testLongDivision() {
        assertEquals(-1, div( 1, -1));
        assertEquals(-1, div(-1,  1));
        assertEquals( 1, div(-1, -1));
        assertEquals( 1, div( 1,  1));
        assertEquals( 0, div( 1,  2));
        assertEquals( 1, div( 3,  2));
        assertEquals( 2, div( 2,  1));
        assertEquals( 3, div( 6,  2));
        assertEquals( 1, div( 0x123456789L, 0x123456789L));
        assertEquals(-0x100000000L, div( 0x200000000L,-2L));
        assertEquals( 0x100000000L, div( 0x200000000L, 2L));
    }

    public static long div(long dividend, long divisor) {
        return dividend / divisor;
    }

    public static void testLongRemainder() {
        assertEquals( 1, rem( 3, -2));
        assertEquals(-1, rem(-3,  2));
        assertEquals(-1, rem(-3, -2));
        assertEquals( 1, rem( 3,  2));
        assertEquals( 0, rem( 1,  1));
        assertEquals( 1, rem( 1,  2));
        assertEquals( 2, rem( 5,  3));
    }

    public static long rem(long dividend, long divisor) {
        return dividend % divisor;
    }

    public static void testLongNegation() {
        assertEquals(-1, neg( 1));
        assertEquals( 0, neg( 0));
        assertEquals( 1, neg(-1));
        assertEquals(0x200000001L, neg(-0x200000001L));
    }

    public static void testLongNegationOverflow() {
        assertEquals(Long.MIN_VALUE, neg(Long.MIN_VALUE));
    }

    public static long neg(long n) {
        return -n;
    }

    public static void testLongLeftShift() {
        assertEquals(1, shl(1, 0));
        assertEquals(2, shl(1, 1));
        assertEquals(4, shl(1, 2));
        assertEquals(Long.MIN_VALUE, shl(1, 31));
    }

    public static void testLongLeftShiftDistanceIsMasked() {
        assertEquals(1, shl(1, 32));
        assertEquals(2, shl(1, 33));
    }

    public static long shl(long value, long distance) {
        return value << distance;
    }

    public static void testLongRightShift() {
        assertEquals(1, shr(1, 0));
        assertEquals(0, shr(1, 1));
        assertEquals(1, shr(2, 1));
        assertEquals(3, shr(15, 2));
        assertEquals(0, shr(Long.MAX_VALUE, 31));
    }

    public static void testLongRightShiftDistanceIsMasked() {
        assertEquals(1, shr(1, 32));
        assertEquals(0, shr(1, 33));
    }

    public static void testLongRightShiftSignExtends() {
        assertEquals(-1, shr(-2, 1));
    }

    public static long shr(long value, long distance) {
        return value >> distance;
    }

    public static void testLongUnsignedRightShift() {
        assertEquals(1, ushr(1, 0));
        assertEquals(0, ushr(1, 1));
        assertEquals(1, ushr(2, 1));
        assertEquals(3, ushr(15, 2));
        assertEquals(0, ushr(Long.MAX_VALUE, 31));
    }

    public static void testLongUnsignedRightShiftDistanceIsMasked() {
        assertEquals(1, ushr(1, 32));
        assertEquals(0, ushr(1, 33));
    }

    public static void testLongUnsignedRightShiftZeroExtends() {
        assertEquals(Long.MAX_VALUE, ushr(-1, 1));
        assertEquals(Long.MAX_VALUE / 2 + 1, ushr(Long.MIN_VALUE, 1));
    }

    public static long ushr(long value, long distance) {
        return value >>> distance;
    }

    public static void testLongBitwiseInclusiveOr() {
        assertEquals(0, or(0, 0));
        assertEquals(1, or(1, 0));
        assertEquals(1, or(0, 1));
        assertEquals(1, or(1, 1));
    }

    public static long or(long value1, long value2) {
        return value1 | value2;
    }

    public static void testLongBitwiseAnd() {
        assertEquals(0, and(0, 0));
        assertEquals(0, and(1, 0));
        assertEquals(0, and(0, 1));
        assertEquals(1, and(1, 1));
    }

    public static long and(long value1, long value2) {
        return value1 & value2;
    }

    public static void testLongBitwiseExclusiveOr() {
        assertEquals(0, xor(0, 0));
        assertEquals(1, xor(1, 0));
        assertEquals(1, xor(0, 1));
        assertEquals(0, xor(1, 1));
    }

    public static long xor(long value1, long value2) {
        return value1 ^ value2;
    }

    public static void testLongIncrementLocalByConstant() {
        assertEquals(-1, lincByMinusOne(0));
        assertEquals( 1, lincByOne(0));
        assertEquals( 2, lincByOne(1));
        assertEquals( 4, lincByTwo(2));
    }

    public static long lincByMinusOne(long value) {
        return value += -1;
    }

    public static long lincByOne(long value) {
        return value += 1;
    }

    public static long lincByTwo(long value) {
        return value += 2;
    }

    public static void main(String[] args) {
        testLongAddition();
        testLongAdditionLocalSlot();
        testLongAdditionOverflow();
        testLongSubtraction();
        testLongSubtractionOverflow();
        testLongSubtractionImmediateLocal();
        testLongMultiplication();
        testLongMultiplicationOverflow();
        testLongDivision();
        testLongRemainder();
        testLongNegation();
        testLongNegationOverflow();
        // FIXME:
        // testLongLeftShift();
        // testLongLeftShiftDistanceIsMasked();
        // testLongRightShift();
        // testLongRightShiftDistanceIsMasked();
        // testLongRightShiftSignExtends();
        // testLongUnsignedRightShift();
        // testLongUnsignedRightShiftDistanceIsMasked();
        // testLongUnsignedRightShiftZeroExtends();
        testLongBitwiseInclusiveOr();
        testLongBitwiseAnd();
        testLongBitwiseExclusiveOr();
        testLongIncrementLocalByConstant();

        exit();
    }
}
