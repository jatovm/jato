/*
 * Copyright (C) 2007  Pekka Enberg
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
public class MethodInvocationAndReturnTest extends TestCase {
    public static void testReturnFromInvokeVirtualReinstatesTheFrameOfTheInvoker() {
        Dispatcher dispatcher = new Dispatcher();
        assertEquals(dispatcher, dispatcher.self);

        InvocationTarget target = new InvocationTarget();
        assertEquals(target, target.self);

        dispatcher.dispatch(target);

        assertEquals(dispatcher, dispatcher.self);
        assertEquals(target, target.self);
    }

    public static class Dispatcher {
        public Object self;

        public Dispatcher() {
            this.self = this;
        }

        public void dispatch(InvocationTarget target) {
            target.invoke();
        }
    }

    public static class InvocationTarget {
        public Object self;

        public InvocationTarget() {
            this.self = this;
        }

        public void invoke() {
        }
    }

    public static void testInvokeVirtualInvokesSuperClassMethodIfMethodIsNotOverridden() {
        SuperClass c = new NonOverridingSubClass();
        assertEquals(0, c.value());
    }

    public static void testInvokeVirtualInvokesSubClassMethodIfMethodIsOverridden() {
        SuperClass c = new OverridingSubClass();
        assertEquals(1, c.value());
    }

    public static class NonOverridingSubClass extends SuperClass {
    }

    public static class OverridingSubClass extends SuperClass {
        public int value() { return 1; }
    }

    public static class SuperClass {
        public int value() { return 0; }
    }

    public static void testRecursiveInvocation() {
        assertEquals(3, recursive(2));
    }

    public static int recursive(int n) {
        if (n == 0)
            return n;

        return n + recursive(n - 1);
    }

    public static void testInvokestaticLongReturnValue() {
        assertEquals(Long.MIN_VALUE, f(Long.MIN_VALUE));
        assertEquals(Long.MAX_VALUE, f(Long.MAX_VALUE));
    }

    public static long f(long x) {
        return x;
    }

    public static void testInvokevirtualLongReturnValue() {
        F f = new F();
        assertEquals(Long.MIN_VALUE, f.f(Long.MIN_VALUE));
        assertEquals(Long.MAX_VALUE, f.f(Long.MAX_VALUE));
    }

    public static class F {
        public long f(long x) {
            return x;
        }
    }

    public static void main(String[] args) {
        testReturnFromInvokeVirtualReinstatesTheFrameOfTheInvoker();
        testInvokeVirtualInvokesSuperClassMethodIfMethodIsNotOverridden();
        testRecursiveInvocation();
        testInvokestaticLongReturnValue();
        testInvokevirtualLongReturnValue();
    }
}
