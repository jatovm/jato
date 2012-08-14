/*
 * Copyright (C) 2009 Pekka Enberg
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

package test.sun.misc;

import java.lang.reflect.Field;
import sun.misc.Unsafe;

import jvm.TestCase;

/**
 * @author Pekka Enberg
 */
public class UnsafeTest extends TestCase {
  private static final Unsafe unsafe = Unsafe.getUnsafe();

  public static void testArrayGetIntVolatile() {
    int[] array = new int[] { 1, Integer.MAX_VALUE, 3 };
    assertEquals(Integer.MAX_VALUE, unsafe.getIntVolatile(array, arrayOffset(array, 1)));
  }

  public static void testArrayGetLongVolatile() {
    long[] array = new long[] { 1, Long.MAX_VALUE, 3 };
    assertEquals(Long.MAX_VALUE, unsafe.getLongVolatile(array, arrayOffset(array, 1)));
  }

  public static void testArrayGetObjectVolatile() {
    Integer[] array = new Integer[] { new Integer(1), new Integer(2), new Integer(3) };
    assertEquals(new Integer(2), unsafe.getObjectVolatile(array, arrayOffset(array, 1)));
  }

  public static void testArrayPutIntVolatile() {
    int[] array = new int[] { 1, 2, 3 };
    unsafe.putIntVolatile(array, arrayOffset(array, 1), Integer.MAX_VALUE);
    assertEquals(Integer.MAX_VALUE, array[1]);
  }

  public static void testArrayPutLong() {
    long[] array = new long[] { 1, 2, 3 };
    unsafe.putLong(array, arrayOffset(array, 1), Long.MAX_VALUE);
    assertEquals(Long.MAX_VALUE, array[1]);
  }

  public static void testArrayPutLongVolatile() {
    long[] array = new long[] { 1, 2, 3 };
    unsafe.putLongVolatile(array, arrayOffset(array, 1), Long.MAX_VALUE);
    assertEquals(Long.MAX_VALUE, array[1]);
  }

  public static void testArrayPutObject() {
    Integer[] array = new Integer[] { new Integer(1), new Integer(2), new Integer(3) };
    unsafe.putObject(array, arrayOffset(array, 1), Integer.MAX_VALUE);
    assertEquals(new Integer(Integer.MAX_VALUE), array[1]);
  }

  public static void testArrayPutObjectVolatile() {
    Integer[] array = new Integer[] { new Integer(1), new Integer(2), new Integer(3) };
    unsafe.putObjectVolatile(array, arrayOffset(array, 1), Integer.MAX_VALUE);
    assertEquals(new Integer(Integer.MAX_VALUE), array[1]);
  }

  public static void testFieldGetLongVolatile() throws Exception {
    UnsafeLongObject obj = new UnsafeLongObject();
    obj.value = Long.MAX_VALUE;
    assertEquals(Long.MAX_VALUE, unsafe.getLongVolatile(obj, fieldOffset(obj, "value")));
  }

  public static void testFieldGetObjectVolatile() throws Exception {
    final String s = "hello, world";
    UnsafeObjectObject obj = new UnsafeObjectObject();
    obj.value = s;
    assertSame(s, unsafe.getObjectVolatile(obj, fieldOffset(obj, "value")));
  }

  public static void testFieldPutIntVolatile() throws Exception {
    UnsafeIntObject obj = new UnsafeIntObject();
    obj.value = Integer.MAX_VALUE;
    unsafe.putIntVolatile(obj, fieldOffset(obj, "value"), Integer.MAX_VALUE);
    assertEquals(Integer.MAX_VALUE, obj.value);
  }

  public static void testFieldPutLong() throws Exception {
    UnsafeLongObject obj = new UnsafeLongObject();
    obj.value = Long.MAX_VALUE;
    unsafe.putLong(obj, fieldOffset(obj, "value"), Long.MAX_VALUE);
    assertEquals(Long.MAX_VALUE, obj.value);
  }

  public static void testFieldPutLongVolatile() throws Exception {
    UnsafeLongObject obj = new UnsafeLongObject();
    obj.value = Long.MAX_VALUE;
    unsafe.putLongVolatile(obj, fieldOffset(obj, "value"), Long.MAX_VALUE);
    assertEquals(Long.MAX_VALUE, obj.value);
  }

  public static void testFieldPutObject() throws Exception {
    UnsafeObjectObject obj = new UnsafeObjectObject();
    obj.value = Integer.MAX_VALUE;
    unsafe.putObject(obj, fieldOffset(obj, "value"), Integer.MAX_VALUE);
    assertEquals(new Integer(Integer.MAX_VALUE), obj.value);
  }

  public static void testFieldPutObjectVolatile() throws Exception {
    final String s = "hello, world";
    UnsafeObjectObject obj = new UnsafeObjectObject();
    obj.value = s;
    unsafe.putObjectVolatile(obj, fieldOffset(obj, "value"), s);
    assertEquals(s, obj.value);
  }

  public static long arrayOffset(Object object, int index) {
    int base = unsafe.arrayBaseOffset(object.getClass());
    int indexScale = unsafe.arrayIndexScale(object.getClass());
    return base + (index * indexScale);
  }

  public static long fieldOffset(Object object, String name) throws Exception {
    Field field = UnsafeObjectObject.class.getDeclaredField(name);
    return unsafe.objectFieldOffset(field);
  }

  public static void testCompareAndSwapInt() throws Exception {
    assertSwapsInt(Integer.MIN_VALUE, Integer.MAX_VALUE);
    assertSwapsInt(Integer.MAX_VALUE, Integer.MIN_VALUE);

    assertDoesNotSwapInt(0, Integer.MIN_VALUE, Integer.MAX_VALUE);
  }

  public static void assertSwapsInt(int expect, int update) throws Exception {
    UnsafeIntObject object = new UnsafeIntObject();
    object.value = expect;
    assertTrue(compareAndSwapInt(object, expect, update));
    assertEquals(update, object.value);
  }

  public static void assertDoesNotSwapInt(int initial, int expect, int update) throws Exception {
    UnsafeIntObject object = new UnsafeIntObject();
    object.value = initial;
    assertFalse(compareAndSwapInt(object, expect, update));
    assertEquals(initial, object.value);
  }

  public static boolean compareAndSwapInt(Object object, int expect, int update) throws Exception {
    Field field = UnsafeIntObject.class.getDeclaredField("value");
    long valueOffset = unsafe.objectFieldOffset(field);
    return unsafe.compareAndSwapInt(object, valueOffset, expect, update);
  }

  public static class UnsafeIntObject {
    public int value;
  }

  public static void testCompareAndSwapLong() throws Exception {
    assertSwapsLong(Long.MIN_VALUE, Long.MAX_VALUE);
    assertSwapsLong(Long.MAX_VALUE, Long.MIN_VALUE);

    assertDoesNotSwapLong(0, Long.MIN_VALUE, Long.MAX_VALUE);
  }

  public static void assertSwapsLong(long expect, long update) throws Exception {
    UnsafeLongObject object = new UnsafeLongObject();
    object.value = expect;
    assertTrue(compareAndSwapLong(object, expect, update));
    assertEquals(update, object.value);
  }

  public static void assertDoesNotSwapLong(long initial, long expect, long update) throws Exception {
    UnsafeLongObject object = new UnsafeLongObject();
    object.value = initial;
    assertFalse(compareAndSwapLong(object, expect, update));
    assertEquals(initial, object.value);
  }

  public static boolean compareAndSwapLong(Object object, long expect, long update) throws Exception {
    Field field = UnsafeLongObject.class.getDeclaredField("value");
    long valueOffset = unsafe.objectFieldOffset(field);
    return unsafe.compareAndSwapLong(object, valueOffset, expect, update);
  }

  public static class UnsafeLongObject {
    public long value;
  }

  public static void testCompareAndSwapObject() throws Exception {
    assertSwapsObject(new Object(), new Object());
    assertSwapsObject(new Object(), new Object());

    assertDoesNotSwapObject(new Object(), new Object(), new Object());
  }

  public static void assertSwapsObject(Object expect, Object update) throws Exception {
    UnsafeObjectObject object = new UnsafeObjectObject();
    object.value = expect;
    assertTrue(compareAndSwapObject(object, expect, update));
    assertEquals(update, object.value);
  }

  public static void assertDoesNotSwapObject(Object initial, Object expect, Object update) throws Exception {
    UnsafeObjectObject object = new UnsafeObjectObject();
    object.value = initial;
    assertFalse(compareAndSwapObject(object, expect, update));
    assertEquals(initial, object.value);
  }

  public static boolean compareAndSwapObject(Object object, Object expect, Object update) throws Exception {
    Field field = UnsafeObjectObject.class.getDeclaredField("value");
    long valueOffset = unsafe.objectFieldOffset(field);
    return unsafe.compareAndSwapObject(object, valueOffset, expect, update);
  }

  public static class UnsafeObjectObject {
    public Object value;
  }

  public static void main(String[] args) throws Exception {
    testArrayGetIntVolatile();
    testArrayGetLongVolatile();
    testArrayGetObjectVolatile();
    testArrayPutIntVolatile();
    testArrayPutLong();
    testArrayPutLongVolatile();
    testArrayPutObject();
    testArrayPutObjectVolatile();
    testFieldGetLongVolatile();
    testFieldGetObjectVolatile();
    testFieldPutIntVolatile();
    testFieldPutLong();
    testFieldPutLongVolatile();
    testFieldPutObject();
    testFieldPutObjectVolatile();
    testCompareAndSwapInt();
    testCompareAndSwapLong();
    testCompareAndSwapObject();
  }
}
