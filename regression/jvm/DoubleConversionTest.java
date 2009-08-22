/*
 * Copyright (C) 2009 Pekka Enberg
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
public class DoubleConversionTest extends TestCase {
    public static double i2d(int val) {
        return (double) val;
    }

    public static void testIntToDoubleConversion() {
        assertEquals(2.0, i2d(2));
        assertEquals(-3000, i2d(-3000));
        assertEquals(-2147483648.0, i2d(-2147483648));
        assertEquals(2147483647.0, i2d(2147483647));
    }

    public static int d2i(double val) {
        return (int) val;
    }

    public static void testDoubleToIntConversion() {
        assertEquals(2, d2i(2.5f));
        assertEquals(-1000, d2i(-1000.0101));
        assertEquals(-2147483648, d2i(-2147483648.0));
//      FIXME:
//      assertEquals(2147483647, d2i(2147483647.0));
    }

    public static double l2d(long val) {
        return (double) val;
    }

    public static void testLongToDoubleConversion() {
        assertEquals(2.0, l2d(2L));
        assertEquals(-3000, l2d(-3000L));
        assertEquals(-2147483648.0, l2d(-2147483648L));
        assertEquals(2147483647.0, l2d(2147483647L));
        assertEquals(-9223372036854775808.0, l2d(-9223372036854775808L));
        assertEquals(9223372036854775807.0, l2d(9223372036854775807L));
    }

    public static long d2l(double val) {
        return (long) val;
    }

    public static void testDoubleToLongConversion() {
        assertEquals(2L, d2l(2.5f));
        assertEquals(-1000L, d2l(-1000.0101));
        assertEquals(-2147483648L, d2l(-2147483648.0));
        assertEquals(2147483647L, d2l(2147483647.0));
        assertEquals(-9223372036854775808L, d2l(-9223372036854775808.0));
//      FIXME:
//      assertEquals(9223372036854775807L, d2l(9223372036854775807.0));
    }

    public static void main(String[] args) {
        testDoubleToIntConversion();
        testIntToDoubleConversion();
        testDoubleToLongConversion();
        testLongToDoubleConversion();
    }
}
