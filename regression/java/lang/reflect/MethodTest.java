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
package java.lang.reflect;

import java.lang.reflect.Modifier;
import jvm.TestCase;

/**
 * @author Pekka Enberg
 */
public class MethodTest extends TestCase {
    public static interface I {
        int x();
    }

    public static class A implements I {
        public int y() {
            return 0;
        }

        public int x() {
            return 1;
        }
    }

    private static void testMethodModifiers() throws Exception {
        assertEquals(Modifier.FINAL | Modifier.PUBLIC, modifiers("publicFinalInstanceMethod"));
        assertEquals(Modifier.STATIC | Modifier.PUBLIC, modifiers("publicClassMethod"));
        assertEquals(Modifier.PUBLIC, modifiers("publicInstanceMethod"));
    }

    private static int modifiers(String name) throws Exception {
      Method m = Klass.class.getMethod(name, new Class[] { });
      return m.getModifiers();
    }

    public static class Klass {
      public final void publicFinalInstanceMethod() { }
      public static void publicClassMethod() { }
      public void publicInstanceMethod() { }

      public static int intIncrement(int x) {
          return x + 1;
      }

      public static long longIncrement(long x) {
          return x + 1;
      }

        public static char charMirror(char x) {
            return x;
        }

        public static boolean boolMirror(boolean x) {
            return x;
        }

    }

    public static Object invoke(String name, Class<?> arg_class, Object arg) {
        try {
            return Klass.class.getMethod(name, new Class[] { arg_class }).invoke(null, new Object[] { arg });
        } catch (Exception e) {
            fail();
            return null;
        }
    }

    public static void testMethodReflectionInvoke() {
        assertEquals(Integer.valueOf(2), invoke("intIncrement", int.class, Integer.valueOf(1)));
        assertEquals(Long.valueOf(0xdeadbeefcafebabfl), invoke("longIncrement", long.class, Long.valueOf(0xdeadbeefcafebabel)));
        assertEquals(Boolean.FALSE, invoke("boolMirror", boolean.class, Boolean.FALSE));
        assertEquals(Boolean.TRUE, invoke("boolMirror", boolean.class, Boolean.TRUE));
        assertEquals(Character.valueOf('x'), invoke("charMirror", char.class, Character.valueOf('x')));
    }

    public static void testInvokeOnInterfaceMethod() {
        A a = new A();
        Object result = null;

        try {
            Method xMethod = I.class.getMethod("x", new Class[0]);
            result = xMethod.invoke(a, new Object[0]);
        } catch (Exception e) {
            fail();
        }

        assertEquals(Integer.valueOf(1), result);
    }

    public static void main(String[] args) throws Exception {
        testMethodModifiers();
        testMethodReflectionInvoke();
        testInvokeOnInterfaceMethod();
    }
}
