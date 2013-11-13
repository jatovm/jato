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
public class PutstaticTest extends TestCase {
    private static class I {
        static int x, y;
        int z;
    };

    private static void testPutStaticConstInt() {
        I.x = 1;
        assertEquals(1, I.x);
    }

    private static void testPutStaticClassFieldInt() {
        I.x = 1;
        I.y = I.x;
        assertEquals(I.x, I.y);
    }

    private static void testPutStaticInstanceFieldInt() {
        I i = new I();
        i.z = 1;
        I.x = i.z;
        assertEquals(i.z, I.x);
    }

    private static void testPutStaticLocalInt() {
        int i = 1;
        I.x = i;
        assertEquals(i, I.x);
    }

    private static class J {
        static long x, y;
        long z;
    };

    private static void testPutStaticConstLong() {
        J.x = 4294967300L;
        assertEquals(4294967300L, J.x);
    }

    private static void testPutStaticClassFieldLong() {
        J.x = 4294967300L;
        J.y = J.x;
        assertEquals(J.x, J.y);
    }

    private static void testPutStaticInstanceFieldLong() {
        J j = new J();
        j.z = 4294967300L;
        J.x = j.z;
        assertEquals(j.z, J.x);
    }

    private static void testPutStaticLocalLong() {
        long j = 4294967300L;
        J.x = j;
        assertEquals(j, J.x);
    }

    private static void testPutStaticConstFloat() {
        FloatClassFields.x = 1.0f;
        assertEquals(1.0f, FloatClassFields.x);
    }

    private static void testPutStaticClassFieldFloat() {
        FloatClassFields.x = 1.0f;
        FloatClassFields.y = FloatClassFields.x;
        assertEquals(FloatClassFields.x, FloatClassFields.y);
    }

    private static void testPutStaticLocalFloat() {
        float x = 1.0f;
        FloatClassFields.x = x;
        assertEquals(x, FloatClassFields.x);
    }

    private static void testPutStaticInstanceFieldFloat() {
        FloatInstanceFields fields = new FloatInstanceFields();
        fields.z = 1.0f;
        FloatClassFields.x = fields.z;
        assertEquals(fields.z, FloatClassFields.x);
    }

    private static class FloatClassFields {
        public static float x, y;
    };

    private static class FloatInstanceFields {
        public float z;
    };

    public static void main(String[] args) {
        testPutStaticConstInt();
        testPutStaticClassFieldInt();
        testPutStaticInstanceFieldInt();
        testPutStaticLocalInt();
        testPutStaticClassFieldLong();
        testPutStaticConstLong();
        testPutStaticInstanceFieldLong();
        testPutStaticLocalLong();
        testPutStaticClassFieldFloat();
        testPutStaticConstFloat();
        testPutStaticInstanceFieldFloat();
        testPutStaticLocalFloat();
    }
}
