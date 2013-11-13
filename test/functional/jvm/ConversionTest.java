/*
 * Copyright (C) 2008  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Arthur Huillet
 */
public class ConversionTest extends TestCase {
    public static int l2i(long value) {
        return (int) value;
    }

    public static void testLongToIntegerConversion() {
        long b = 10;
        long c = b + 0x100000000L;
        assertEquals(10, l2i(b));
        assertEquals(10, l2i(c));
    }

    public static long i2l(int value) {
        return (long) value;
    }

    public static void testIntegerToLongConversion() {
        int b = 10;
        int c = -1;
        assertEquals(10L, i2l(b));
        assertEquals(-1L, i2l(c));
    }

    public static byte i2b(int value) {
        return (byte) value;
    }

    public static void testIntegerToByteConversion() {
        assertEquals(-1, i2b(-1));
        assertEquals(0, i2b(0));
        assertEquals(1, i2b(1));
        assertEquals(Byte.MAX_VALUE, i2b(Byte.MAX_VALUE));
        assertEquals(-1, i2b(Integer.MAX_VALUE));
    }

    public static char i2c(int value) {
        return (char) value;
    }

    public static void testIntegerToCharConversion() {
        assertEquals(Character.MAX_VALUE, i2c(-1));
        assertEquals(0, i2c(0));
        assertEquals(1, i2c(1));
        assertEquals(Character.MAX_VALUE, i2c(Character.MAX_VALUE));
        assertEquals(Character.MAX_VALUE, i2c(Integer.MAX_VALUE));
    }

    public static short i2s(int value) {
        return (short) value;
    }

    public static void testIntegerToShortConversion() {
        assertEquals(-1, i2s(-1));
        assertEquals(0, i2s(0));
        assertEquals(1, i2s(1));
        assertEquals(Short.MAX_VALUE, i2s(Short.MAX_VALUE));
        assertEquals(-1, i2s(Integer.MAX_VALUE));
    }

    public static char b2c(byte b) {
        return (char) b;
    }

    public static void testByteToCharConversion() {
        assertEquals(0, b2c((byte) 0));
        assertEquals((char) 0x7f, b2c((byte) 0x7f));
        assertEquals((char) 0xffc0, b2c((byte) -0x40));
    }

    public static void testConversionFromIntPushesInt() {
        char c1, c2;
        int i = 1;

        // conversion from int to byte should push int onto mimic stack
        // so the following expression should not trigger byte -> char
        // and short -> char conversion.
        c1 = (char) (byte) i;
        c2 = (char) (short) i;

        takeInt(c1);
        takeInt(c2);
    }

    public static void main(String[] args) {
        testLongToIntegerConversion();
        testIntegerToLongConversion();
        testIntegerToByteConversion();
        testIntegerToCharConversion();
        testIntegerToShortConversion();
        testByteToCharConversion();
        testConversionFromIntPushesInt();
    }
}
