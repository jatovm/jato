package jvm;

/**
 * @author Tomek Grabiec <tgrabiec@gmail.com>
 * @author Vegard Nossum <vegard.nossum@gmail.com>
 */
public class VirtualAbstractInterfaceMethodTest extends TestCase {
    private static interface X {
        public void x();
    };

    private static abstract class A implements X {
    };

    private static boolean ok = false;

    private static class B extends A {
        public void x() {
            ok = true;
        }
    };

    public static void main(String[] args) {
        A a = new B();
        a.x();

        assertTrue(ok);
    }
}
