package jvm;

class BranchTest extends TestCase {
    static boolean t;
    static boolean f;

    private static void ok() {
    }

    private static void testAndBranch() {
        if (t && t)
            ok();
        else
            fail();

        if (t && f)
            fail();
        else
            ok();

        if (f && t)
            fail();
        else
            ok();

        if (f && f)
            fail();
        else
            ok();
    }

    private static void testOrBranch() {
        if (t || t)
            ok();
        else
            fail();

        if (t || f)
            ok();
        else
            fail();

        if (f || t)
            ok();
        else
            fail();

        if (f || f)
            fail();
        else
            ok();
    }

    public static void main(String[] args) {
        /* Try to work around the optimizing compiler */
        t = true;
        f = false;

        testAndBranch();
        testOrBranch();

        exit();
    }
}
