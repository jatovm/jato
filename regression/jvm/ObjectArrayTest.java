package jvm;

/**
 * @author Vegard Nossum
 */
public class ObjectArrayTest extends TestCase {
    private static class X {
        public static class Y {
        }

        Y[] y;
    }

    private static void testObjectArrayCreation() {
        X x = new X();
        x.y = new X.Y[1];
    }

    public static void main(String args[]) {
        testObjectArrayCreation();
    }
}
