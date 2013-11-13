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
public class IntegerArithmeticExceptionsTest extends TestCase {
    public static void testIntegerDivisionByZeroRegReg() {
        int x = 3;

        try {
            x = 1 / 0;
        } catch (ArithmeticException e) {}

        assertEquals(3, x);
    }

    public static void testIdivThrowsAritmeticException() {
        boolean caught = false;
        int denominator = 0;

        try {
            takeInt(1 / denominator);
        } catch (ArithmeticException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testIremThrowsAritmeticException() {
        boolean caught = false;
        int denominator = 0;

        try {
            takeInt(1 % denominator);
        } catch (ArithmeticException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String[] args) {
        testIntegerDivisionByZeroRegReg();
        testIdivThrowsAritmeticException();
        testIremThrowsAritmeticException();
    }
}
