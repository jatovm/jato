package jvm;

public class InterfaceFieldInheritanceTest extends TestCase {
    private interface A {
        Object x = "foo";
    }

    private static class B implements A {
    }

    public static void main(String[] args) {
        assertTrue(B.x.equals("foo"));
    }
}
