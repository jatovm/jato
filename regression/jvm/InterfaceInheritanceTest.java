package jvm;

/**
 * @author Tomasz Grabiec
 * @author Vegard Nossum
 */
public class InterfaceInheritanceTest extends TestCase {
    private interface A {
        void x();
    };

    private interface B extends A {
    };

    private static B getNullB() {
        return null;
    }

    static public void main( String [] args ) {
        B b = getNullB();

        try {
            b.x();
            fail();
        } catch (NullPointerException e) {
            /* This should happen */
        }
    }
}
