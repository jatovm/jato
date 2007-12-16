/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

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

    public static void main(String[] args) {
        testReturnFromInvokeVirtualReinstatesTheFrameOfTheInvoker();
        Runtime.getRuntime().halt(retval);
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
}
