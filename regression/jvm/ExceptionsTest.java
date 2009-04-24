/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jvm;

/**
 * @author Tomasz Grabiec
 */
public class ExceptionsTest extends TestCase {
    public static void testCatchCompilation() {
        int i;

        i = 0;
        try {
            i = 2;
        } catch (Exception e) {
            i = 1;
        }

        assertEquals(i, 2);
    }

    public static void main(String args[]) {
        testCatchCompilation();

        Runtime.getRuntime().halt(retval);
    }
};
