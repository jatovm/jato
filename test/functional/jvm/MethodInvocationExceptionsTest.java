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
public class MethodInvocationExceptionsTest extends TestCase {
    private void privateMethod() {
    }

    public void publicMethod() {
    }

    private static MethodInvocationExceptionsTest getNullMethodInvocationExceptionsTest() {
        return null;
    }

    public static void testInvokespecialThrowsNullPointerException() {
        boolean caught = false;
        MethodInvocationExceptionsTest test = getNullMethodInvocationExceptionsTest();

        try {
            test.privateMethod();
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testInvokevirtualThrowsNullPointerException() {
        boolean caught = false;
        MethodInvocationExceptionsTest test = getNullMethodInvocationExceptionsTest();

        try {
            test.publicMethod();
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static native void nativeMethod();

    public static void testInvokespecialThrowsUnsatisfiedLinkError() {
        boolean caught = false;

        try {
            nativeMethod();
        } catch (UnsatisfiedLinkError e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String args[]) {
        testInvokespecialThrowsNullPointerException();
        testInvokevirtualThrowsNullPointerException();
        testInvokespecialThrowsUnsatisfiedLinkError();
    }
}
