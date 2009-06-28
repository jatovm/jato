package jvm;

/**
 * This once turned up a bug where the array size check would interfere
 * with the assignment when the array was a member of an object.
 *
 * The bug was discovered by Vegard and fixed by Tomek.
 */
public class ArrayMemberTest {
    public static void main(String[] args) {
        X x = new X();

        int sum = 0;
        sum += x.a[0];
    }

    private static class X {
        public char[] a = new char[1];
    }
}
