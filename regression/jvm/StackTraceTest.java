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

import jato.internal.VM;

/**
 * @author Tomasz Grabiec
 */
public class StackTraceTest extends TestCase {

    public static void doTestJITStackTrace() {
        StackTraceElement []st = new Exception().getStackTrace();

        assertNotNull(st);
        assertEquals(3, st.length);

        assertStackTraceElement(st[0], 36, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "doTestJITStackTrace",
                false);

        assertStackTraceElement(st[1], 53, "StackTraceTest.java",
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
            VM.println(null);
        } catch (NullPointerException e) {
            st = e.getStackTrace();
        }

        assertNotNull(st);
        assertEquals(4, st.length);

        assertStackTraceElement(st[0], -1, null, "jato.internal.VM",
                "println", true);

        assertStackTraceElement(st[1], 60, "StackTraceTest.java",
                "jvm.StackTraceTest",
                "doTestVMNativeInStackTrace",
                false);

        assertStackTraceElement(st[2], 83, "StackTraceTest.java",
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
                "nativeMethod", true);

        assertStackTraceElement(st[1], 92, "StackTraceTest.java",
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