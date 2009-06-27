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
public class ObjectStackTest extends TestCase {
    /*
     * The following method evaluates whether n > 1 is true ("0x01") or false
     * ("0x00") and passes the result to the assertTrue() method. The generated
     * bytecode will have two mutually exclusive basic blocks to handle the
     * evaluation. We're testing here that the object stack state is properly
     * preserved during JIT compilation.
     */
    public static void assertIsGreaterThanOne(int n) {
        assertTrue(n > 1);
    }

    /*
     * The inverse case of the above.
     */
    public static void assertIsNotGreaterThanOne(int n) {
        assertFalse(n > 1);
    }

    public static void testObjectStackWhenBranching() {
        assertIsGreaterThanOne(2);
        assertIsNotGreaterThanOne(1);
    }

    public static void testLoadAndIncrementLocal() {
        int x = 0;

        assertEquals(0, x++);
    }

    public static void main(String[] args) {
        testObjectStackWhenBranching();
        testLoadAndIncrementLocal();

        exit();
    }
}
