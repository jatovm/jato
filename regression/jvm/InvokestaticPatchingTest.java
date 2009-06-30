package jvm;

public class InvokestaticPatchingTest extends TestCase {
    static boolean clinit_run = false;

    private static class X {
        static {
            clinit_run = true;
        }

        static void f() {
        }
    }

    public static void main(String[] args) {
        int i = 0;

        /* Should not trap, therefore clinit_run remains false */
        assertFalse(clinit_run);
        X x = null;
        assertFalse(clinit_run);

        /* Should trap, therefore clinit_run becomes true */
        X.f();
        assertTrue(clinit_run);

        clinit_run = false;
        /* Should not trap, therefore clinit_run remains false */
        X.f();
        assertFalse(clinit_run);

        exit();
    }
}
