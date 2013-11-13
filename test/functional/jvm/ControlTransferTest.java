/*
 * Copyright (C) 2008  Saeed Siam
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Saeed Siam
 */
public class ControlTransferTest extends TestCase {
    public static void testLabeledBreak() {
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

    public static void testForLoop() {
        int i;

        for (i = 0; i < 10; i++)
            ;

        assertEquals(i, 10);
    }

    public static boolean ifeq(int x) {
        return x == 0;
    }

    public static void testIfEq() {
        assertFalse(ifeq(1));
        assertTrue(ifeq(0));
    }

    public static boolean ifne(int x) {
        return x != 0;
    }

    public static void testIfNe() {
        assertTrue(ifne(1));
        assertFalse(ifne(0));
    }

    public static boolean iflt(int x) {
        return x < 0;
    }

    public static void testIfLt() {
        assertTrue(iflt(-1));
        assertFalse(iflt(0));
        assertFalse(iflt(1));
    }

    public static boolean ifle(int x) {
        return x <= 0;
    }

    public static void testIfLe() {
        assertTrue(ifle(-1));
        assertTrue(ifle(0));
        assertFalse(ifle(1));
    }

    public static boolean ifgt(int x) {
        return x > 0;
    }

    public static void testIfGt() {
        assertFalse(ifgt(-1));
        assertFalse(ifgt(0));
        assertTrue(ifgt(1));
    }

    public static boolean ifge(int x) {
        return x >= 0;
    }

    public static void testIfGe() {
        assertFalse(ifge(-1));
        assertTrue(ifge(0));
        assertTrue(ifge(1));
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

    public static boolean if_acmpeq(Object x, Object y) {
        return x == y;
    }

    public static void testIfAcmpEq() {
        Object x = new Object();
        Object y = new Object();
        Object z = x;
        assertTrue(if_acmpeq(x, z));
        assertFalse(if_acmpeq(x, y));
    }

    public static boolean if_acmpne(Object x, Object y) {
        return x != y;
    }

    public static void testIfAcmpNe() {
        Object x = new Object();
        Object y = new Object();
        Object z = x;
        assertFalse(if_acmpne(x, z));
        assertTrue(if_acmpne(x, y));
    }

    public static boolean if_lcmpeq(long x, long y) {
        return x == y;	/* lcmp + ifne */
    }

    public static void testIfLcmpEq() {
        assertFalse(if_lcmpeq(0, 1));
        assertFalse(if_lcmpeq(Long.MIN_VALUE, 1));
        assertTrue(if_lcmpeq(0, 0));
        assertTrue(if_lcmpeq(Long.MIN_VALUE, Long.MIN_VALUE));
        assertTrue(if_lcmpeq(Long.MAX_VALUE, Long.MAX_VALUE));
    }

    public static boolean if_lcmpgt(long x, long y) {
	return x > y;	/* lcmp + ifle */
    }

    public static void testIfLcmpGt() {
        assertTrue(if_lcmpgt(1, 0));
        assertTrue(if_lcmpgt(Long.MAX_VALUE, 1));
        assertTrue(if_lcmpgt(Long.MAX_VALUE, Long.MIN_VALUE));
        assertFalse(if_lcmpgt(1, 1));
        assertFalse(if_lcmpgt(1, Long.MAX_VALUE));
    }

    /* See Section 7.5 "More Control Examples" of the JVM spec for details.  */
    public static boolean lessThan100(float f) {
        if (f < 100.0f) {
            return true;
        } else {
            return false;
        }
    }

    public static void testFcmpg() {
        assertFalse(lessThan100(Float.NaN));
        assertFalse(lessThan100(101.0f));
        assertTrue(lessThan100(99.0f));
    }

    /* See Section 7.5 "More Control Examples" of the JVM spec for details.  */
    public static boolean greaterThan100(float f) {
        if (f > 100.0f) {
            return true;
        } else {
            return false;
        }
    }

    public static void testFcmpl() {
        assertFalse(greaterThan100(Float.NaN));
        assertTrue(greaterThan100(101.0f));
        assertFalse(greaterThan100(99.0f));
    }

    /* See Section 7.5 "More Control Examples" of the JVM spec for details.  */
    public static boolean lessThan100(double d) {
        if (d < 100.0) {
            return true;
        } else {
            return false;
        }
    }

    public static void testDcmpg() {
        assertFalse(lessThan100(Double.NaN));
        assertFalse(lessThan100(101.0));
        assertTrue(lessThan100(99.0));
    }

    /* See Section 7.5 "More Control Examples" of the JVM spec for details.  */
    public static boolean greaterThan100(double d) {
        if (d > 100.0) {
            return true;
        } else {
            return false;
        }
    }

    public static void testDcmpl() {
        assertFalse(greaterThan100(Double.NaN));
        assertTrue(greaterThan100(101.0));
        assertFalse(greaterThan100(99.0));
    }

    public static void main(String [] args) {
        testLabeledBreak();
        testForLoop();
        testIfEq();
        testIfNe();
        testIfLt();
        testIfLe();
        testIfGt();
        testIfGe();
        testIfIcmpEq();
        testIfIcmpNe();
        testIfIcmpLt();
        testIfIcmpGt();
        testIfIcmpLe();
        testIfIcmpGe();
        testIfAcmpEq();
        testIfAcmpNe();
        testIfLcmpEq();
        testIfLcmpGt();
        testFcmpg();
        testFcmpl();
        testDcmpg();
        testDcmpl();
    }
}
