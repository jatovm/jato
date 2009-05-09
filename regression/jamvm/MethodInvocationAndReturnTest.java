/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

import jvm.TestCase;

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

    public static void main(String[] args) {
        testReturnFromInvokeVirtualReinstatesTheFrameOfTheInvoker();
        testInvokeVirtualInvokesSuperClassMethodIfMethodIsNotOverridden();
        testRecursiveInvocation();

        Runtime.getRuntime().halt(retval);
    }
}
