/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */
package jvm;

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

    public static void testIsInstanceOf() {
        ClassFields obj = new ClassFields();
        assertTrue(obj instanceof ClassFields);
        assertTrue(obj instanceof Object);
        assertFalse(null instanceof Object);
    }

    public static void testByteArrayLoadAndStore() {
        byte[] array = new byte[5];
        array[1] = 1;
        assertEquals(array[1], 1);
    }

    public static void testCharArrayLoadAndStore() {
        char[] array = new char[5];
        array[1] = 65535;
        assertEquals(array[1], 65535);
    }

    public static void testShortArrayLoadAndStore() {
        short[] array = new short[5];
        array[3] = 1;
        assertEquals(array[3], 1);
    }

    public static void testIntArrayLoadAndStore() {
        int[] array = new int[5];
        array[1] = 1;
        assertEquals(1, array[1]);
    }

    public static void testObjectArrayLoadAndStore() {
        InitializingClass[] array = new InitializingClass[5];
        InitializingClass obj = new InitializingClass(1);
        array[2] = obj;
        assertEquals(array[2], obj);
    }

    public static void testMultiDimensionalArrayLoadAndStore() {
        int[][] array = new int[5][5];
        array[0][0] = 0;
    }

    public static Object getClassFieldsAndIncrementField() {
        ClassFields.field++;
        return new ClassFields();
    }

    public static void testCheckCast() {
        Object object = new InstanceFields();

        InstanceFields instanceFields = (InstanceFields) null;
        assertNull(instanceFields);

        instanceFields = (InstanceFields) object;
        assertNotNull(instanceFields);

	/* Test for expression double evaluation bug */
        ClassFields.field = 0;

        ClassFields classFields =
            (ClassFields)getClassFieldsAndIncrementField();
        assertEquals(1, ClassFields.field);
        assertNotNull(classFields);

        //Following test will fail.

        /*
         ClassFields classField = new ClassFields();
         object = classField;
         instanceFields = (InstanceFields) object;
        */

    }

    private static int sideEffectCounter;

    private static int sideEffect() {
        sideEffectCounter++;
        return 0;
    }

    private static void testArrayLoadSideEffectBug() {
        int array[] = {0, 0};
        int x;

        sideEffectCounter = 0;
        x = array[sideEffect()];

        assertEquals(0, x);
        assertEquals(1, sideEffectCounter);
    }

    private static void testArrayStoreSideEffectBug() {
        int array[] = {0, 0};
        int x = 1;

        sideEffectCounter = 0;
        array[sideEffect()] = x;

        assertEquals(1, sideEffectCounter);
    }

    public static void main(String[] args) {
        testNewObject();
        testObjectInitialization();
        testInstanceFieldAccess();
        testNewArray();
        testANewArray();
        testArrayLength();
        testMultiANewArray();
        testIsInstanceOf();
        testIntArrayLoadAndStore();
        testCharArrayLoadAndStore();
        testByteArrayLoadAndStore();
        testShortArrayLoadAndStore();
        testObjectArrayLoadAndStore();
        testMultiDimensionalArrayLoadAndStore();
        testCheckCast();
        testArrayLoadSideEffectBug();
        testArrayStoreSideEffectBug();
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
