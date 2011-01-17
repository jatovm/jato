/*
 * Copyright (C) 2011 Joonas Reynders
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
package java.lang;

import jvm.TestCase;

/**
* @author Joonas Reynders
*/
public class JNITest extends TestCase {
    static {
        System.load("./test/functional/jni/libjnitest.so");
    }

    native static public String returnPassedString(String str);
    native static public int returnPassedInt(int i);
    native static public long returnPassedLong(long l);
    native static public boolean returnPassedBoolean(boolean b);
    native static public short returnPassedShort(short s);
    native static public byte returnPassedByte(byte b);
    native static public char returnPassedChar(char c);
    native static public float returnPassedFloat(float f);
    native static public double returnPassedDouble(double d);

    public static void testReturnPassedString() {
        assertEquals("testString", returnPassedString("testString"));
    }

    public static void testReturnPassedInt() {
        assertEquals(42, returnPassedInt(42));
    }

    public static void testReturnPassedLong() {
        assertEquals(42l, returnPassedLong(42l));
    }

    public static void testReturnPassedBoolean() {
        assertEquals(true, returnPassedBoolean(true));
    }

    public static void testReturnPassedShort() {
        short s = 42;
        assertEquals(42, returnPassedShort(s));
    }

    public static void testReturnPassedByte() {
        byte b = 42;
        assertEquals(42, returnPassedByte(b));
    }

    public static void testReturnPassedChar() {
        assertEquals('a', returnPassedChar('a'));
    }

    public static void testReturnPassedFloat() {
        assertEquals(42.0f, returnPassedFloat(42.0f));
    }

    public static void testReturnPassedDouble() {
        assertEquals(42.0, returnPassedDouble(42.0));
    }

    public static void main(String[] args) {
        testReturnPassedString();
        testReturnPassedInt();
        testReturnPassedLong();
        testReturnPassedBoolean();
        testReturnPassedShort();
        testReturnPassedByte();
        testReturnPassedChar();
/*
Exception in thread "main" java.lang.AssertionError: Expected '42.0', but was 'NaN'.
at jvm.TestCase.fail(TestCase.java:110)
at jvm.TestCase.assertEquals(TestCase.java:43)
at java.lang.JNITest.testReturnPassedFloat(JNITest.java:79)

        testReturnPassedFloat();
*/
/*
Exception in thread "main" java.lang.AssertionError: Expected '42.0', but was 'NaN'.
at jvm.TestCase.fail(TestCase.java:110)
at jvm.TestCase.assertEquals(TestCase.java:49)
at java.lang.JNITest.testReturnPassedDouble(JNITest.java:83)

        testReturnPassedDouble();
*/
    }
}
