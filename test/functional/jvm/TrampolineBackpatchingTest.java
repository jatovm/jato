/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Tomasz Grabiec
 */
public class TrampolineBackpatchingTest extends TestCase {
    public static int staticMethod(int x) {
        return x+x;
    }

    private class A {
        public int virtualMethod(int x) {
            return x+x;
        }

        private int privateMethod(int x) {
            return x+x;
        }

        public int invokePrivate(int x) {
            return privateMethod(x);
        }
    }

    private class B extends A {
        public int invokeSuper(int x) {
            return super.virtualMethod(x);
        }

        public int invokeVirtual(int x) {
            return virtualMethod(x);
        }
    }

    public static void testInvokestatic() {
        assertEquals(staticMethod(4), 8);
    }

    public  void testInvokevirtual() {
        assertEquals(new A().virtualMethod(4), 8);
        assertEquals(new B().invokeVirtual(4), 8);
    }

    public  void testInvokespecial() {
        assertEquals(new A().invokePrivate(4), 8);
        assertEquals(new B().invokeSuper(4), 8);
    }

    public static void main(String [] args) {
        TrampolineBackpatchingTest t = new TrampolineBackpatchingTest();

        testInvokestatic();
        t.testInvokevirtual();
        t.testInvokespecial();

        /*
         * Run tests again to check if all call sites have
         * been properly fixed.
         */
        testInvokestatic();
        t.testInvokevirtual();
        t.testInvokespecial();
    }
}
