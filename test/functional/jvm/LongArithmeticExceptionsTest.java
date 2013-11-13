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
public class LongArithmeticExceptionsTest extends TestCase {
    public static void testLdivThrowsAritmeticException() {
        boolean caught = false;
        long denominator = 0;

        try {
            takeLong(1L / denominator);
        } catch (ArithmeticException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void testLremThrowsAritmeticException() {
        boolean caught = false;
        long denominator = 0;

        try {
            takeLong(1L % denominator);
        } catch (ArithmeticException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String[] args) {
        testLdivThrowsAritmeticException();
        testLremThrowsAritmeticException();
    }
}
