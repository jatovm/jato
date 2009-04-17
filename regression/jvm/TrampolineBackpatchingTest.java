/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
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

        Runtime.getRuntime().halt(retval);
    }
}
