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

public class RegisterAllocatorTortureTest extends TestCase {
    public static int one() {
      return 1;
    }

    public static void testIntegerBigExpression() {
        int result;

        /* Yes, this is ugly. But it generates tons of register pressure.  */
        result = ((((((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one()))) + (((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one())))) + ((((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one()))) + (((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one()))))) + (((((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one()))) + (((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one())))) + ((((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one()))) + (((1 + one()) + (1 + one())) + ((1 + one()) + (1 + one()))))));

        assertEquals(64, result);
    }

    public static void testComplexRegisterAllocatorPressure() {
        /* This is a nice test case because it crashes.  */
        new C().substring(1, 2);
    }

    private static class C {
        private char[] value;

        public C() {
            this.value = new char[0];
        }

        public C(boolean d) {
            if (!d)
                jato.internal.VM.exit(1);
        }

        public C substring(int a, int b) {
            int len = b - a;
            return new C((len << 2) >= value.length);
        }
    }

    static class TestClass {
        public int get(int index) {
            return index;
        }
    };

    private static int doTestIntervalSplitOnRegisterAllocatorPressure(TestClass obj, int index) {
        return obj.get(index) + obj.get(index) + obj.get(index);
    }

    public static void testIntervalSplitOnRegisterAllocatorPressure() {
        TestClass obj = new TestClass();
        doTestIntervalSplitOnRegisterAllocatorPressure(obj, 0);
    }

    static class DataFlowTestClass {
        private void test2(int a, int b, int c, int d, int e, int f, int g, int h, int i) {
            assertEquals(0, a);
            assertEquals(1, b);
            assertEquals(2, c);
            assertEquals(3, d);
            assertEquals(4, e);
            assertEquals(5, f);
            assertEquals(6, g);
            assertEquals(1, h);
            assertEquals(7, i);
        }

        public void test(int a, int b, int c, int d, int e, int f, int g, int h){
            test2(a, b, c, d, e, f, g, ((f != 0) ? 1 : 0), h);
        }
    };

    public static void testDataFlowResolution() {
        DataFlowTestClass x = new DataFlowTestClass();

        x.test(0, 1, 2, 3, 4, 5, 6, 7);
    }

    public static void main(String[] args) {
      testIntegerBigExpression();
      testComplexRegisterAllocatorPressure();
      testIntervalSplitOnRegisterAllocatorPressure();
      testDataFlowResolution();
    }
}
