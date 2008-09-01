/*
 * Copyright (C) 2008  Saeed Siam
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */
package jamvm;

/**
 * @author Saeed Siam
 */
public class ControlTransferTest extends TestCase {
    public static void testGoTo() {
        int i = 0;

        outer: {
            inner: {
                if (i == 0)
                    break inner;
                fail(/*Should not reach here.*/);
            }
            if (i == 0)
                break outer;
            fail(/*Should not reach here.*/);
        }
    }

    public static boolean if_icmpeq(int x, int y) {
        return x == y;
    }

    public static void testIfIcmpEq() {
        assertFalse(if_icmpeq(0, 1));
        assertTrue(if_icmpeq(1, 1));
    }

    public static boolean if_icmpne(int x, int y) {
        return x != y;
    }

    public static void testIfIcmpNe() {
        assertFalse(if_icmpne(1, 1));
        assertTrue(if_icmpne(0, 1));
    }

    public static boolean if_icmplt(int x, int y) {
        return x < y;
    }

    public static void testIfIcmpLt() {
        assertFalse(if_icmplt(1, 1));
        assertFalse(if_icmplt(1, 0));
        assertTrue(if_icmplt(0, 1));
    }

    public static boolean if_icmpgt(int x, int y) {
        return x > y;
    }

    public static void testIfIcmpGt() {
        assertFalse(if_icmpgt(1, 1));
        assertFalse(if_icmpgt(0, 1));
        assertTrue(if_icmpgt(1, 0));
    }

    public static boolean if_icmple(int x, int y) {
        return x <= y;
    }

    public static void testIfIcmpLe() {
        assertTrue(if_icmple(1, 1));
        assertTrue(if_icmple(0, 1));
        assertFalse(if_icmple(1, 0));
    }

    public static boolean if_icmpge(int x, int y) {
        return x >= y;
    }

    public static void testIfIcmpGe() {
        assertTrue(if_icmpge(1, 1));
        assertFalse(if_icmpge(0, 1));
        assertTrue(if_icmpge(1, 0));
    }

    public static void main(String [] args) {
        testIfIcmpEq();
        testIfIcmpNe();
        testIfIcmpLt();
        testIfIcmpGt();
        testIfIcmpLe();
        testIfIcmpGe();
        testGoTo();

        Runtime.getRuntime().halt(retval);
    }
}
