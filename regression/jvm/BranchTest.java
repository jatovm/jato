package jvm;

class BranchTest extends TestCase {
    static boolean t,  f;
    static boolean t1, t2;
    static boolean f1, f2;

    private static void ok() {
    }

    private static void testAndBranch() {
        if (t1 && t2)
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

        if (f1 && f2)
            fail();
        else
            ok();
    }

    private static void testOrBranch() {
        if (t1 || t2)
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

        if (f1 || f2)
            fail();
        else
            ok();
    }

    public static void main(String[] args) {
        /* Try to work around the optimizing compiler */
        t = t1 = t2 = true;
        f = f1 = f2 = false;

        testAndBranch();
        testOrBranch();
    }
}
