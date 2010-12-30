package jvm;

public class PutstaticPatchingTest extends TestCase {
    static boolean clinit_run = false;

    private static class X {
        @SuppressWarnings("unused") static int x;
        @SuppressWarnings("unused") static int y;

        static {
            x = 1;
            y = 2;
            clinit_run = true;
        }
    }

    private static void testClassInitOnPutstatic() {
        int i = 0;

        assertFalse(clinit_run);
        /* Should trap, therefore clinit_run becomes true */
        X.x = i;
        assertTrue(clinit_run);

        clinit_run = false;
        /* Should not trap, therefore clinit_run remains false */
        X.x = i;
        X.y = i;
        assertFalse(clinit_run);
    }

    private static class DoubleFieldClass {
        public static double x;
    };

    private static void testDoublePutstaticPatching() {
        DoubleFieldClass.x = 1.0;
        assertEquals(DoubleFieldClass.x, 1.0);
    }

    private static class FloatFieldClass {
        public static float x;
    };

    private static void testFloatPutstaticPatching() {
        FloatFieldClass.x = 1.0f;
        assertEquals(FloatFieldClass.x, 1.0f);
    }

    private static class IntFieldClass {
        public static int x;
    };

    private static void testIntPutstaticPatching() {
        IntFieldClass.x = 1;
        assertEquals(IntFieldClass.x, 1);
    }

    public static void main(String[] args) {
        testClassInitOnPutstatic();
        testDoublePutstaticPatching();
        testFloatPutstaticPatching();
        testIntPutstaticPatching();
    }
}
