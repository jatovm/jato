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
package java.lang;

import jvm.TestCase;

/**
 * @author Pekka Enberg
 */
public class VMClassTest extends TestCase {
    public static void testGetDeclaredAnnotations() {
        assertEquals(0, VMClass.getDeclaredAnnotations(Object.class).length);
    }

    public static void testGetEnclosingClass() {
        assertNull(Object.class.getEnclosingClass());
        class LocalClass { }
        assertEquals(VMClassTest.class, LocalClass.class.getEnclosingClass());
    }

    public static void testIsAnonymousClass() {
        assertFalse(VMClass.isAnonymousClass(int.class));
        assertFalse(VMClass.isAnonymousClass(int[].class));
        assertFalse(VMClass.isAnonymousClass(Object.class));
        assertFalse(VMClass.isAnonymousClass(Object[].class));
        assertTrue(VMClass.isAnonymousClass(new Object() { }.getClass()));
    }

    public static void testIsArray() {
        assertFalse(VMClass.isArray(int.class));
        assertTrue(VMClass.isArray(int[].class));
        assertFalse(VMClass.isArray(Object.class));
        assertTrue(VMClass.isArray(Object[].class));
        assertFalse(VMClass.isArray(new Object() { }.getClass()));
    }

    public static void testIsLocalClass() {
        assertFalse(VMClass.isLocalClass(int.class));
        assertFalse(VMClass.isLocalClass(int[].class));
        assertFalse(VMClass.isLocalClass(Object.class));
        assertFalse(VMClass.isLocalClass(Object[].class));
        assertFalse(VMClass.isLocalClass(new Object() { }.getClass()));
        class LocalClass { }
        assertTrue(VMClass.isLocalClass(LocalClass.class));
    }

    public static void testIsMemberClass() {
        assertFalse(VMClass.isMemberClass(Object.class));
        assertTrue(VMClass.isMemberClass(MemberClass.class));
    }
    class MemberClass { }

    public static void testIsPrimitive() {
        assertTrue(VMClass.isPrimitive(int.class));
        assertFalse(VMClass.isPrimitive(int[].class));
        assertFalse(VMClass.isPrimitive(Object.class));
        assertFalse(VMClass.isPrimitive(Object[].class));
        assertFalse(VMClass.isPrimitive(new Object() { }.getClass()));
    }

    public static void main(String[] args) {
        testGetDeclaredAnnotations();
        testIsAnonymousClass();
        testIsArray();
        testIsLocalClass();
        testIsMemberClass();
        testIsPrimitive();
    }
}
