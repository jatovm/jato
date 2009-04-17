/*
 * Copyright (C) 2008  Saeed Siam
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

import jvm.TestCase;

/**
 * @author Saeed Siam
 */
public class SynchronizationTest extends TestCase {
    public static void testMonitorEnterAndExit() {
        Object obj = new Object();
        synchronized (obj) {
            assertNotNull(obj);
        }
    }

    public static void main(String[] args) {
        testMonitorEnterAndExit();

        Runtime.getRuntime().halt(retval);
    }
}
