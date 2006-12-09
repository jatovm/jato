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
        assertEquals(0, ClassFields.field);
        ClassFields.field = 1;
        assertEquals(1, ClassFields.field);
    }

    public static void testInstanceFieldAccess() {
        InstanceFields fields = new InstanceFields();
        assertEquals(0, fields.field);
        fields.field = 1;
        assertEquals(1, fields.field);
    }

    public static void main(String[] args) {
        testNewObject();
        testClassFieldAccess();
        testInstanceFieldAccess();

        Runtime.getRuntime().halt(retval);
    }

    private static class ClassFields {
        public static int field;
    };

    private static class InstanceFields {
        public int field;
    };
}
