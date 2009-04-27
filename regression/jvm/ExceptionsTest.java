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
        assertEquals(2, onlyTryBlockExecuted());
    }

    public static int onlyTryBlockExecuted() {
        int i = 0;

        try {
            i = 2;
        } catch (Exception e) {
            i--;
            return i;
        } catch (Throwable e) {
            i++;
        }

        return i;
    }

    public static void main(String args[]) {
        Runtime.getRuntime().halt(retval);
    }
};
