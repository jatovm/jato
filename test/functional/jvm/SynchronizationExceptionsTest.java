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
public class SynchronizationExceptionsTest extends TestCase {
    public static void testMonitorenterThrowsNullPointerException() {
        boolean caught = false;
        Object mon = null;

        try {
            synchronized(mon) {
                takeInt(1);
            }
        } catch (NullPointerException e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String args[]) {
        testMonitorenterThrowsNullPointerException();
        /* TODO: testMonitorexit() */
    }
}
