/*
 * Copyright (C) 2008  Pekka Enberg
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

    public static void main(String[] args) {
        testLongToIntegerConversion();
        testIntegerToLongConversion();
        testIntegerToByteConversion();
        testIntegerToCharConversion();
        testIntegerToShortConversion();
        testByteToCharConversion();
    }
}
