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
public class ArrayExceptionsTest extends TestCase {
    private static Object[] getNullObjectArray() {
        return null;
    }

    public static void testArrayLoadThrowsNullPointerException() {
        boolean caught = false;
        Object[] array = getNullObjectArray();

        try {
            takeObject(array[0]);
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testArrayLoadThrowsArrayIndexOutOfBoundsException() {
        boolean caught = false;
        Object[] array = new String[3];

        try {
            takeObject(array[3]);
        } catch (ArrayIndexOutOfBoundsException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testArrayLoad() {
        testArrayLoadThrowsNullPointerException();
        testArrayLoadThrowsArrayIndexOutOfBoundsException();
    }

    public static void testArrayStoreThrowsNullPointerException() {
        boolean caught = false;
        Object[] array = getNullObjectArray();

        try {
            array[0] = null;
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testArrayStoreThrowsArrayIndexOutOfBoundsException() {
        boolean caught = false;
        Object[] array = new String[3];

        try {
            array[3] = "test";
        } catch (ArrayIndexOutOfBoundsException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testArrayStoreThrowsArrayStoreException() {
        boolean caught = false;
        Object[] array = new String[3];

        try {
            array[2] = new ArrayExceptionsTest();
        } catch (ArrayStoreException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testArrayStore() {
        testArrayStoreThrowsNullPointerException();
        testArrayStoreThrowsArrayIndexOutOfBoundsException();
        testArrayStoreThrowsArrayStoreException();
    }

    public static void testArraylengthThrowsNullPointerException() {
        boolean caught = false;
        Object[] array = getNullObjectArray();

        try {
            takeInt(array.length);
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testAnewarrayThrowsNegativeArraySizeException() {
        boolean caught = false;
        Object array[] = null;

        try {
            array = new Object[-1];
        } catch (NegativeArraySizeException e) {
            caught = true;
        }

        assertTrue(caught);

        takeObject(array);
    }

    public static void testNewarrayThrowsNegativeArraySizeException() {
        boolean caught = false;
        int array[] = null;

        try {
            array = new int[-1];
        } catch (NegativeArraySizeException e) {
            caught = true;
        }

        assertTrue(caught);

        takeObject(array);
    }

    public static void testMultianewarrayThrowsNegativeArraySizeException() {
        boolean caught = false;
        Object array[][] = null;

        try {
            array = new Object[1][-1];
        } catch (NegativeArraySizeException e) {
            caught = true;
        }

        assertTrue(caught);

        caught = false;
        try {
            array = new Object[-1][1];
        } catch (NegativeArraySizeException e) {
            caught = true;
        }

        assertTrue(caught);

        takeObject(array);
    }

    public static void testArrayBoundsCheck() {
        boolean caught = false;
        int array[] = { 0, 1, 2 };

        try {
            takeInt(array[-1]);
        } catch (ArrayIndexOutOfBoundsException e) {
            caught = true;
        }

        assertTrue(caught);

        caught = false;

        try {
            takeInt(array[3]);
        } catch (ArrayIndexOutOfBoundsException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String args[]) {
        testArrayLoad();
        testArrayStore();
        testArraylengthThrowsNullPointerException();
        testAnewarrayThrowsNegativeArraySizeException();
        testNewarrayThrowsNegativeArraySizeException();
        testMultianewarrayThrowsNegativeArraySizeException();
        testArrayBoundsCheck();
    }
}
