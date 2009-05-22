/*
 * Copyright (C) 2009 Pekka Enberg
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
public class PutfieldTest extends TestCase {
    static class I {
        int x, y;
        static int z;
    };

    public static void testPutFieldConstInt() {
        I i = new I();
        i.x = 1;
        assertEquals(1, i.x);
    }

    public static void testPutFieldInstanceFieldInt() {
        I i = new I();
        i.x = 1;
        i.y = i.x;
        assertEquals(i.x, i.y);
    }

    public static void testPutFieldClassFieldInt() {
        I i = new I();
        I.z = 1;
        i.x = I.z;
        assertEquals(I.z, i.x);
    }

    public static void testPutFieldLocalInt() {
        I i = new I();
        int l = 1;
        i.x = l;
        assertEquals(l, i.x);
    }

    static class J {
        long x, y;
        static long z;
    };

    public static void testPutFieldConstLong() {
        J j = new J();
        j.x = 4294967300L;
        assertEquals(4294967300L, j.x);
    }

    public static void testPutFieldInstanceFieldLong() {
        J j = new J();
        j.x = 4294967300L;
        j.y = j.x;
        assertEquals(j.x, j.y);
    }

    public static void testPutFieldClassFieldLong() {
        J j = new J();
        J.z = 4294967300L;
        j.x = J.z;
        assertEquals(J.z, j.x);
    }

    public static void testPutFieldLocalLong() {
        J j = new J();
        long l = 4294967300L;
        j.x = l;
        assertEquals(l, j.x);
    }

    public static void main(String[] args) {
        testPutFieldConstInt();
        testPutFieldInstanceFieldInt();
        testPutFieldClassFieldInt();
        testPutFieldLocalInt();
        testPutFieldConstLong();
        testPutFieldInstanceFieldLong();
        testPutFieldClassFieldLong();
        testPutFieldLocalLong();

        exit();
    }
}
