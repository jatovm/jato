/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

/**
 * @author Pekka Enberg
 */
public class LoadConstantsTest extends TestCase {
    public static void testIconst() {
        assertEquals(-1, iconst_m1());
        assertEquals( 0, iconst_0());
        assertEquals( 1, iconst_1());
        assertEquals( 2, iconst_2());
        assertEquals( 3, iconst_3());
        assertEquals( 4, iconst_4());
        assertEquals( 5, iconst_5());
    }

    public static int iconst_m1() {
        return -1;
    }

    public static int iconst_0() {
        return 0;
    }

    public static int iconst_1() {
        return 1;
    }

    public static int iconst_2() {
        return 2;
    }

    public static int iconst_3() {
        return 3;
    }

    public static int iconst_4() {
        return 4;
    }

    public static int iconst_5() {
        return 5;
    }

    public static void main(String[] args) {
        testIconst();

        Runtime.getRuntime().halt(retval);
    }
}
