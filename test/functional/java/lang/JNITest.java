/*
 * Copyright (C) 2011 Joonas Reynders
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *   Linking this library statically or dynamically with other modules is
 *   making a combined work based on this library. Thus, the terms and
 *   conditions of the GNU General Public License cover the whole
 *   combination.
 *
 *   As a special exception, the copyright holders of this library give you
 *   permission to link this library with independent modules to produce an
 *   executable, regardless of the license terms of these independent
 *   modules, and to copy and distribute the resulting executable under terms
 *   of your choice, provided that you also meet, for each linked independent
 *   module, the terms and conditions of the license of that module. An
 *   independent module is a module which is not derived from or based on
 *   this library. If you modify this library, you may extend this exception
 *   to your version of the library, but you are not obligated to do so. If
 *   you do not wish to do so, delete this exception statement from your
 *   version.
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

  native static public String[] staticReturnPassedStringArray(String[] str);
  native static public int[] staticReturnPassedIntArray(int[] i);
  native static public long[] staticReturnPassedLongArray(long[] l);
  native static public boolean[] staticReturnPassedBooleanArray(boolean[] b);
  native static public short[] staticReturnPassedShortArray(short[] s);
  native static public byte[] staticReturnPassedByteArray(byte[] b);
  native static public char[] staticReturnPassedCharArray(char[] c);
  native static public float[] staticReturnPassedFloatArray(float[] f);
  native static public double[] staticReturnPassedDoubleArray(double[] d);

  native public String returnPassedString(String str);
  native public int returnPassedInt(int i);
  native public long returnPassedLong(long l);
  native public boolean returnPassedBoolean(boolean b);
  native public short returnPassedShort(short s);
  native public byte returnPassedByte(byte b);
  native public char returnPassedChar(char c);
  native public float returnPassedFloat(float f);
  native public double returnPassedDouble(double d);

  native public String[] returnPassedStringArray(String[] str);
  native public int[] returnPassedIntArray(int[] i);
  native public long[] returnPassedLongArray(long[] l);
  native public boolean[] returnPassedBooleanArray(boolean[] b);
  native public short[] returnPassedShortArray(short[] s);
  native public byte[] returnPassedByteArray(byte[] b);
  native public char[] returnPassedCharArray(char[] c);
  native public float[] returnPassedFloatArray(float[] f);
  native public double[] returnPassedDoubleArray(double[] d);

  native static public String staticToUpper(String str);

  native static public boolean staticTestJNIEnvIsInitializedCorrectly();

  // JNI 1.6 native test functions
  native static public int staticGetVersion();
  native static public Class<Object> staticDefineClass(String name, ClassLoader classloader);

  private static JNITest jniTest = new JNITest();

  public static void testReturnPassedString() {
    assertEquals("testString", staticReturnPassedString("testString"));
    assertEquals("testString", jniTest.returnPassedString("testString"));
    assertEquals("testString", staticReturnPassedStringArray(new String[]{"testString"})[0]);
    assertEquals("testString", jniTest.returnPassedStringArray(new String[]{"testString"})[0]);
  }

  public static void testReturnPassedInt() {
    assertEquals(42, staticReturnPassedInt(42));
    assertEquals(42, jniTest.returnPassedInt(42));
    assertEquals(42, staticReturnPassedIntArray(new int[]{42})[0]);
    assertEquals(42, jniTest.returnPassedIntArray(new int[]{42})[0]);
  }

  public static void testReturnPassedLong() {
    assertEquals(42l, staticReturnPassedLong(42l));
    assertEquals(42l, jniTest.returnPassedLong(42l));
    assertEquals(42l, staticReturnPassedLongArray(new long[]{42l})[0]);
    assertEquals(42l, jniTest.returnPassedLongArray(new long[]{42l})[0]);
  }

  public static void testReturnPassedBoolean() {
    assertEquals(true, staticReturnPassedBoolean(true));
    assertEquals(true, jniTest.returnPassedBoolean(true));
    assertEquals(true, staticReturnPassedBooleanArray(new boolean[]{true})[0]);
    assertEquals(true, jniTest.returnPassedBooleanArray(new boolean[]{true})[0]);
  }

  public static void testReturnPassedShort() {
    short s = 42;
    assertEquals(42, staticReturnPassedShort(s));
    assertEquals(42, jniTest.returnPassedShort(s));
    assertEquals(42, staticReturnPassedShortArray(new short[]{s})[0]);
    assertEquals(42, jniTest.returnPassedShortArray(new short[]{s})[0]);
  }

  public static void testReturnPassedByte() {
    byte b = 42;
    assertEquals(42, staticReturnPassedByte(b));
    assertEquals(42, jniTest.returnPassedByte(b));
    assertEquals(42, staticReturnPassedByteArray(new byte[]{b})[0]);
    assertEquals(42, jniTest.returnPassedByteArray(new byte[]{b})[0]);
  }

  public static void testReturnPassedChar() {
    assertEquals('a', staticReturnPassedChar('a'));
    assertEquals('a', jniTest.returnPassedChar('a'));
    assertEquals('a', staticReturnPassedCharArray(new char[]{'a'})[0]);
    assertEquals('a', jniTest.returnPassedCharArray(new char[]{'a'})[0]);
  }

  public static void testReturnPassedFloat() {
    assertEquals(42.0f, staticReturnPassedFloat(42.0f));
    assertEquals(42.0f, jniTest.returnPassedFloat(42.0f));
    assertEquals(42.0f, staticReturnPassedFloatArray(new float[]{42.0f})[0]);
    assertEquals(42.0f, jniTest.returnPassedFloatArray(new float[]{42.0f})[0]);
  }

  public static void testReturnPassedDouble() {
    assertEquals(42.0, staticReturnPassedDouble(42.0));
    assertEquals(42.0, jniTest.returnPassedDouble(42.0));
    assertEquals(42.0, staticReturnPassedDoubleArray(new double[]{42.0})[0]);
    assertEquals(42.0, jniTest.returnPassedDoubleArray(new double[]{42.0})[0]);
  }

  public static void testStringManipulation() {
    assertEquals("TESTSTRING", staticToUpper("testString"));
  }

  public static void testJNIEnvIsInitializedCorrectly() {
    assertTrue(staticTestJNIEnvIsInitializedCorrectly());
  }

  // JNI 1.6 API function tests
  public static void testGetVersion() {
    assertEquals(0x00010006, staticGetVersion());
  }

  public static void testDefineClass() {
    Class<Object> jniTestFixtureClass = staticDefineClass("test/functional/jni/JNITestFixture.class", ClassLoader.getSystemClassLoader());
    assertNotNull(jniTestFixtureClass);
    assertEquals("class jni.JNITestFixture", jniTestFixtureClass.toString());

    try {
      Object jniTestFixtureInstance = jniTestFixtureClass.newInstance();
      assertEquals("jni.JNITestFixture@", jniTestFixtureInstance.toString().substring(0, "jni.JNITestFixture@".length()));
      assertEquals("TESTSTRING", jniTestFixtureInstance.getClass().getMethod("toUpper", new Class[]{String.class}).invoke(null, "testString"));
    } catch (Exception e) {
      throw new RuntimeException(e);
    }
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
    testStringManipulation();
    testJNIEnvIsInitializedCorrectly();
    testGetVersion();
    testDefineClass();
  }
}
