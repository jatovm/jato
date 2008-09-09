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

    public static void main(String[] args) {
        testLongToIntegerConversion();

        Runtime.getRuntime().halt(retval);
    }
}
