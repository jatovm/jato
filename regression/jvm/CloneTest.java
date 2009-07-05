package jvm;

public class CloneTest extends TestCase {
    public static class X implements Cloneable {
        int x;
        int y;

        public Object clone() throws CloneNotSupportedException {
            return super.clone();
        }
    }

    private static void cloneObjectTest() throws Exception {
        X x = new X();
        x.x = 1;
        x.y = 2;

        X y = (X) x.clone();
        assertEquals(y.x, 1);
        assertEquals(y.y, 2);
    }

    private static void clonePrimitiveArrayTest() throws Exception {
        int[] x = new int[10];

        for (int i = 0; i < x.length; ++i)
            x[i] = i;

        int[] y = (int[]) x.clone();

        for (int i = 0; i < x.length; ++i)
            assertEquals(y[i], i);

        y[0]++;
        assertFalse(x[0] == y[0]);
    }

    private static void cloneObjectArrayTest() throws Exception {
        X[] x = new X[10];

        for (int i = 0; i < x.length; ++i) {
            x[i] = new X();
            x[i].x = i;
            x[i].y = 2 * i;
        }

        X[] y = (X[]) x.clone();

        for (int i = 0; i < x.length; ++i) {
            assertEquals(y[i].x, i);
            assertEquals(y[i].y, 2 * i);
        }

        y[0] = new X();
        assertFalse(x[0] == y[0]);
    }

    public static void main(String[] args) throws Exception {
        cloneObjectTest();
        clonePrimitiveArrayTest();
        cloneObjectArrayTest();
        exit();
    }
}
