package jvm;

public class PutstaticPatchingTest extends TestCase {
    static boolean clinit_run = false;

    private static class X {
        static int x;
        static int y;

        static {
            x = 1;
            y = 2;
            clinit_run = true;
        }
    }

    public static void main(String[] args) {
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
}
