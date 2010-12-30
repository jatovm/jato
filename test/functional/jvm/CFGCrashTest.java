package jvm;

public class CFGCrashTest extends TestCase {
    private CFGCrashTest[] x;

    public CFGCrashTest(boolean unused) {
        if (x == null)
            x = null;

        x = (x == null ? null : null);
    }

    public static void main(String[] args) {
        new CFGCrashTest(false);
    }
}
