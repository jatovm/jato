/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

import jato.internal.VM;

/**
 * @author Tomasz Grabiec
 */
public class ClassExceptionsTest extends TestCase {
    static class A {
        public static int field;
    };

    public static void testClassInitFailure() {
        boolean caught = false;

        VM.enableFault(VM.FAULT_IN_CLASS_INIT, "jvm/ClassExceptionsTest$A");

        try {
            A.field = 0;
        } catch (ExceptionInInitializerError e) {
            caught = true;
        }

        VM.disableFault(VM.FAULT_IN_CLASS_INIT);

        assertTrue(caught);

        caught = false;
        try {
            A.field = 0;
        } catch (NoClassDefFoundError e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String []args) {
        testClassInitFailure();
    }
}
