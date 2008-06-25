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

    public static void testObjectInitialization() {
        InitializingClass obj = new InitializingClass(1);
        assertEquals(1, obj.value);
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

    public static void testNewArray() {
        int[] array = null;
        assertNull(array);
        array = new int[5];
        assertNotNull(array);
    }

    public static void testANewArray() {
        Object[] array = null;
        assertNull(array);
        array = new Object[255];
        assertNotNull(array);
    }

    public static void testArrayLength() {
        Object[] array = new Object[255];
        assertEquals(255, array.length);
    }

    public static void testMultiANewArray() {
       Object[][] array = null;
       assertNull(array);
       array = new Object[5][4];
       assertNotNull(array);
       assertEquals(5, array.length);
    }

    public static void main(String[] args) {
        testNewObject();
        testObjectInitialization();
        testClassFieldAccess();
        testInstanceFieldAccess();
        testNewArray();
        testANewArray();
        testArrayLength();
        testMultiANewArray();

        Runtime.getRuntime().halt(retval);
    }

    private static class InitializingClass {
        public int value;

        public InitializingClass(int value) {
            this.value = value;
	}
    };

    private static class ClassFields {
        public static int field;
    };

    private static class InstanceFields {
        public int field;
    };
}
