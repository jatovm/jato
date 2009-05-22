/*
 * Copyright (C) 2009 Tomasz Grabiec
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
 * @author Tomasz Grabiec
 */
public class TrampolineBackpatchingTest extends TestCase {

    public static int test(int x) {
        return x+x;
    }

    public static int testBackpatchingOnTheFly(int x) {
        /* No trampoline call should be generated for this,
           because method test() is already compiled */
        return test(x);
    }

    static void testVTableBackpatching() {
        /* Instantiating RuntimeException calls virtual method
           Throwable.fillInStackTrace.  This tests proper functioning
           of vtable back-patching */
        new RuntimeException("foo");
    }

    public int testArg(int a) {
        return a+1;
    }

    public static void main(String [] args) {
        int x;
        TrampolineBackpatchingTest t = new TrampolineBackpatchingTest();

        /* Test backpatching of multiple call sites
           at once */
        x = 1;
        x = test(x);
        x = test(x);
        x = test(x);
        assertEquals(x, 8);

        x = testBackpatchingOnTheFly(8);
        assertEquals(x, 16);

        testVTableBackpatching();

        /* Another invokevirtual backpatching test */
        x = 1;
        x = t.testArg(x);
        x = t.testArg(x);
        assertEquals(x, 3);

        exit();
    }
}
