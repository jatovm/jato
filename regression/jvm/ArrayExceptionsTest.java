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

    public static void main(String args[]) {
        testArrayLoad();
        testArrayStore();
        testArraylengthThrowsNullPointerException();
        testAnewarrayThrowsNegativeArraySizeException();
        testNewarrayThrowsNegativeArraySizeException();
        testMultianewarrayThrowsNegativeArraySizeException();
    }
}
