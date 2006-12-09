/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

/**
 * @author Pekka Enberg
 */
public class ObjectCreationAndManipulationTest extends TestCase {
    public static void testNewObject() {
        Object obj = null;
	assertNull(obj);
	obj = new Object();
	assertNotNull(obj);
    }

    public static void testClassFieldAccess() {
        assertEquals(0, StaticFields.field);
        StaticFields.field = 1;
        assertEquals(1, StaticFields.field);
    }

    public static void main(String[] args) {
        testNewObject();
        testClassFieldAccess();

        Runtime.getRuntime().halt(retval);
    }

    private static class StaticFields {
        public static int field;
    };
}
