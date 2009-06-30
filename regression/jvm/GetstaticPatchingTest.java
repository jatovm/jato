package jvm;

public class GetstaticPatchingTest extends TestCase {
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
        assertFalse(clinit_run);
        /* Should trap, therefore clinit_run becomes true */
        assertEquals(1, X.x);
        assertTrue(clinit_run);

        clinit_run = false;
        /* Should not trap, therefore clinit_run remains false */
        assertEquals(1, X.x);
        assertEquals(2, X.y);
        assertFalse(clinit_run);

        exit();
    }
}
