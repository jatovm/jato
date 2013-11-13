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
public class StackTraceTest extends TestCase {

    public static void doTestJITStackTrace() {
        StackTraceElement []st = new Exception().getStackTrace();

        assertNotNull(st);
        assertEquals(3, st.length);

        assertStackTraceElement(st[0], 17, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "doTestJITStackTrace",
                false);

        assertStackTraceElement(st[1], 34, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "testJITStackTrace",
                false);
    }

    public static void testJITStackTrace() {
        doTestJITStackTrace();
    }

    public static void doTestVMNativeInStackTrace() {
        StackTraceElement []st = null;

        try {
            VM.throwNullPointerException();
        } catch (NullPointerException e) {
            st = e.getStackTrace();
        }

        assertNotNull(st);
        assertEquals(4, st.length);

        assertStackTraceElement(st[0], -1, null, "jato.internal.VM",
                "throwNullPointerException", true);

        assertStackTraceElement(st[1], 41, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "doTestVMNativeInStackTrace",
                false);

        assertStackTraceElement(st[2], 64, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "testVMNativeInStackTrace",
                false);
    }

    public static void testVMNativeInStackTrace() {
        doTestVMNativeInStackTrace();
    }

    public static native void nativeMethod();

    public static void testJNIUnsatisfiedLinkErrorStackTrace() {
        StackTraceElement st[] = null;

        try {
            nativeMethod();
        } catch (UnsatisfiedLinkError e) {
            st = e.getStackTrace();
        }

        assertNotNull(st);
        assertEquals(3, st.length);

        assertStackTraceElement(st[0], -1, null,
                "jvm.StackTraceTest",
                "nativeMethod",
                true);

        assertStackTraceElement(st[1], 73, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "testJNIUnsatisfiedLinkErrorStackTrace",
                false);
    }

    public static void main(String []args) {
        testJITStackTrace();
        testVMNativeInStackTrace();
        testJNIUnsatisfiedLinkErrorStackTrace();
    }
}
