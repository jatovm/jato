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
public class ObjectCreationAndManipulationExceptionsTest extends TestCase {
    private static class X {
        public int field;
    }

    private static X getNullX() {
        return null;
    }

    public static void testGetfieldThrowsNullPointerException() {
        boolean caught = false;
        X test = getNullX();

        try {
            takeInt(test.field);
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testPutfieldThrowsNullPointerException() {
        boolean caught = false;
        X test = getNullX();

        try {
            test.field = 1;
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testCheckcastThrowsClassCastException() {
        boolean caught = false;
        Object o = null;
        String s = null;

        try {
            s = (String)o;
        } catch (ClassCastException e) {
            caught = true;
        }

        assertFalse(caught);

        o = new Object();

        try {
            s = (String)o;
        } catch (ClassCastException e) {
            caught = true;
        }

        assertTrue(caught);

        takeObject(s);
    }

    public static void main(String args[]) {
        testGetfieldThrowsNullPointerException();
        testPutfieldThrowsNullPointerException();
        testCheckcastThrowsClassCastException();
    }
}
