/*
 * Copyright (C) 2009 Vegard Nossum
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Vegard Nossum
 */
public class ArrayTest extends TestCase {
    static byte[] sb = new byte[0];

    private static void testEmptyStaticArrayLength() {
        assertEquals(sb.length, 0);
    }

    byte[] b = new byte[0];

    private static void testEmptyArrayLength() {
        ArrayTest x = new ArrayTest();
        assertEquals(x.b.length, 0);
    }

    static byte[] sb2 = new byte[1];

    private static void testStaticArrayLength() {
        assertEquals(sb2.length, 1);
    }

    byte[] b2 = new byte[1];

    private static void testArrayLength() {
        ArrayTest x = new ArrayTest();
        assertEquals(x.b2.length, 1);
    }

    private static void testIntegerElementLoadStore() {
        int array[] = new int[2];

        array[1] = 2;
        array[0] = 1;

        assertEquals(1, array[0]);
        assertEquals(2, array[1]);
    }

    private static void testByteElementLoadStore() {
        byte array[] = new byte[2];

        array[1] = 2;
        array[0] = 1;

        assertEquals(1, array[0]);
        assertEquals(2, array[1]);
    }

    private static void testCharElementLoadStore() {
        char array[] = new char[2];

        array[1] = 'a';
        array[0] = 'b';

        assertEquals('b', array[0]);
        assertEquals('a', array[1]);
    }

    private static void testBooleanElementLoadStore() {
        boolean array[] = new boolean[2];

        array[1] = true;
        array[0] = false;

        assertFalse(array[0]);
        assertTrue(array[1]);
    }

    private static void testShortElementLoadStore() {
        short array[] = new short[2];

        array[1] = 2;
        array[0] = 1;

        assertEquals(1, array[0]);
        assertEquals(2, array[1]);
    }

    public static void testLongElementLoadStore() {
        long array[] = new long[2];

        array[1] = 2L;
        array[0] = 1L;

        assertEquals(1L, array[0]);
        assertEquals(2L, array[1]);
    }

    private static void testReferenceElementLoadStore() {
        Object array[] = new Object[2];
        Object a = "a";
        Object b = "b";

        array[1] = a;
        array[0] = b;

        assertEquals(b, array[0]);
        assertEquals(a, array[1]);
    }

    private static void testFloatElementLoadStore() {
        float array[] = new float[2];
        float a = 1.34567f;
        float b = -5.2311f;

        array[1] = a;
        array[0] = b;

        assertEquals(b, array[0]);
        assertEquals(a, array[1]);
    }

    private static void testDoubleElementLoadStore() {
	double array[] = new double[2];
        double a = 1.34567;
        double b = -5.2311;

        array[1] = a;
        array[0] = b;

        assertEquals(b, array[0]);
        assertEquals(a, array[1]);
    }

    public static void testArrayClass() {
        int big_arr[][][]  = new int[2][2][2];

        assertClassName("[[[I", big_arr);
        assertClassName("[[I", big_arr[0]);
        assertClassName("[I", big_arr[0][0]);

        int arr2[] = new int[10];

        assertTrue(arr2.getClass().equals(big_arr[0][0].getClass()));
    }

    public static void testByteCharShortLoadPushesInt() {
        char c[] = { 0 };
        byte b[] = { 0 };
        short s[] = { 0 };

        // caload, baload, saload should push INT on stack. If
        // they don't the following will trigger assertion
        // failure in convert_truncation().
        takeInt( (byte) c[0] );
        takeInt( (char) b[0] );
        takeInt( (byte) s[0] );
    }

    public static void main(String args[]) {
        testEmptyStaticArrayLength();
        testEmptyArrayLength();

        testStaticArrayLength();
        testArrayLength();

        testIntegerElementLoadStore();
        testBooleanElementLoadStore();
        testCharElementLoadStore();
        testByteElementLoadStore();
        testShortElementLoadStore();
        testLongElementLoadStore();
        testReferenceElementLoadStore();
        testFloatElementLoadStore();
        testDoubleElementLoadStore();

        testArrayClass();
        testByteCharShortLoadPushesInt();
    }
}
