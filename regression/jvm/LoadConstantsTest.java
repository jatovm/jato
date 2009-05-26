/*
 * Copyright (C) 2006  Pekka Enberg
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
public class LoadConstantsTest extends TestCase {
    public static void testAconstNull() {
        assertNull(aconst_null());
    }

    public static Object aconst_null() {
        return null;
    }

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

    public static void testLdcStringConstant() {
        assertEquals(4, "HELO".length());
    } 

    public static void main(String[] args) {
        testAconstNull();
        testIconst();
        testLdcStringConstant();

        exit();
    }
}
