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
package jvm;

import jato.internal.VM;

/**
 * @author Tomasz Grabiec
 */
public class ClassExceptionsTest extends TestCase {
    static class A {
        public static int field;
    };

    public static void testClassInitFailure() {
        boolean caught = false;

        VM.enableFault(VM.FAULT_IN_CLASS_INIT, "ClassExceptionsTest$A");

        try {
            A.field = 0;
        } catch (ExceptionInInitializerError e) {
            caught = true;
        }

        VM.disableFault(VM.FAULT_IN_CLASS_INIT);

        assertTrue(caught);

        caught = false;
        try {
            A.field = 0;
        } catch (NoClassDefFoundError e) {
            caught = true;
        }

        assertTrue(caught);
    }

    public static void main(String []args) {
/* FIXME
        testClassInitFailure();
*/
    }
}
