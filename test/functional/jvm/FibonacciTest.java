/*
 * Copyright (C) 2009 Pekka Enberg
 * 
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Pekka Enberg
 */
public class FibonacciTest extends TestCase {
    public static void testFibonacci() {
        assertEquals(1, fib(1));
        assertEquals(1, fib(2));
        assertEquals(2, fib(3));
        assertEquals(3, fib(4));
        assertEquals(5, fib(5));
    }

    public static int fib(int n) {
        if (n <= 2)
            return 1;

        return fib(n-1) + fib(n-2);
    }

    public static void main(String[] args) {
        testFibonacci();
    }
}
