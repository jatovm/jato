/*
 * Copyright (C) 2008  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

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

    public static void main(String[] args) {
        testLongToIntegerConversion();
        testIntegerToLongConversion();

        Runtime.getRuntime().halt(retval);
    }
}
