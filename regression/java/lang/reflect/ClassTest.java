/*
 * Copyright (C) 2009 Tomasz Grabiec
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
package java.lang.reflect;

import java.lang.reflect.Modifier;
import jvm.TestCase;

/**
 * @author Tomasz Grabiec
 */
public class ClassTest extends TestCase {
    public static void testPrimitiveClassModifiers() {
        Class<?> primitive = Integer.TYPE;
        int modifiers = primitive.getModifiers();

        assertEquals(Modifier.PUBLIC
                 | Modifier.FINAL
                 | Modifier.ABSTRACT,
                 modifiers);
    }

    private static class X {
    };

    public static void testArrayClassModifiers() {
        X[] array = new X[1];

        int modifiers = array.getClass().getModifiers();

        /*
         * FIXME: hotspot sets ACC_PRIVATE and ACC_STATIC for X while
         * jato does not.
         */
        assertEquals(0 /* Modifier.PRIVATE | Modifier.STATIC */,
                     X.class.getModifiers());

        assertEquals(0 /* Modifier.PRIVATE | Modifier.STATIC */
                     | Modifier.FINAL
                     | Modifier.ABSTRACT,
                     modifiers);
    }

    public static void testRegularClassModifiers() {
        assertEquals(Modifier.PUBLIC, ClassTest.class.getModifiers());
    }

    public static void testGetDeclaringClass() {
        assertNull(Object.class.getDeclaringClass());
        assertEquals(ClassTest.class, X.class.getDeclaringClass());
    }

    public static void main(String []args) {
        testPrimitiveClassModifiers();
        testArrayClassModifiers();
        testRegularClassModifiers();
        testGetDeclaringClass();
    }
}
