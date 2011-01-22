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

    native static public String staticReturnPassedString(String str);
    native static public int staticReturnPassedInt(int i);
    native static public long staticReturnPassedLong(long l);
    native static public boolean staticReturnPassedBoolean(boolean b);
    native static public short staticReturnPassedShort(short s);
    native static public byte staticReturnPassedByte(byte b);
    native static public char staticReturnPassedChar(char c);
    native static public float staticReturnPassedFloat(float f);
    native static public double staticReturnPassedDouble(double d);

    native public String returnPassedString(String str);
    native public int returnPassedInt(int i);
    native public long returnPassedLong(long l);
    native public boolean returnPassedBoolean(boolean b);
    native public short returnPassedShort(short s);
    native public byte returnPassedByte(byte b);
    native public char returnPassedChar(char c);
    native public float returnPassedFloat(float f);
    native public double returnPassedDouble(double d);

    private static JNITest jniTest = new JNITest();

    public static void testReturnPassedString() {
        assertEquals("testString", staticReturnPassedString("testString"));
        assertEquals("testString", jniTest.returnPassedString("testString"));

    }

    public static void testReturnPassedInt() {
        assertEquals(42, staticReturnPassedInt(42));
        assertEquals(42, jniTest.returnPassedInt(42));
    }

    public static void testReturnPassedLong() {
        assertEquals(42l, staticReturnPassedLong(42l));
        assertEquals(42l, jniTest.returnPassedLong(42l));
    }

    public static void testReturnPassedBoolean() {
        assertEquals(true, staticReturnPassedBoolean(true));
        assertEquals(true, jniTest.returnPassedBoolean(true));
    }

    public static void testReturnPassedShort() {
        short s = 42;
        assertEquals(42, staticReturnPassedShort(s));
        assertEquals(42, jniTest.returnPassedShort(s));
    }

    public static void testReturnPassedByte() {
        byte b = 42;
        assertEquals(42, staticReturnPassedByte(b));
        assertEquals(42, jniTest.returnPassedByte(b));
    }

    public static void testReturnPassedChar() {
        assertEquals('a', staticReturnPassedChar('a'));
        assertEquals('a', jniTest.returnPassedChar('a'));
    }

    public static void testReturnPassedFloat() {
        assertEquals(42.0f, staticReturnPassedFloat(42.0f));
        assertEquals(42.0f, jniTest.returnPassedFloat(42.0f));
    }

    public static void testReturnPassedDouble() {
        assertEquals(42.0, staticReturnPassedDouble(42.0));
        assertEquals(42.0, jniTest.returnPassedDouble(42.0));
    }

    public static void main(String[] args) {
        testReturnPassedString();
        testReturnPassedInt();
        testReturnPassedLong();
        testReturnPassedBoolean();
        testReturnPassedShort();
        testReturnPassedByte();
        testReturnPassedChar();
        testReturnPassedFloat();
        testReturnPassedDouble();
    }
}
