/*
 * Copyright (C) 2006-2011 Pekka Enberg
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */
package jvm;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class TestCase {
    protected static void assertArrayEquals(byte[] expected, byte[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(char[] expected, char[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(short[] expected, short[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(int[] expected, int[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(long[] expected, long[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(float[] expected, float[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(double[] expected, double[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertArrayEquals(Object[] expected, Object[] actual) {
        if (expected == null && actual == null)
            return;

        assertNotNull(expected);
        assertNotNull(actual);
        assertEquals(expected.length, actual.length);
        for (int i = 0; i < expected.length; i++)
            assertEquals(expected[i], actual[i]);
    }

    protected static void assertEquals(int expected, int actual) {
        if (expected != actual) {
            fail("Expected '" + expected + "', but was '" + actual + "'.");
        }
    }

    public static void assertEquals(long expected, long actual) {
        if (expected != actual) {
            fail("Expected '" + expected + "', but was '" + actual + "'.");
        }
    }

    public static void assertEquals(float expected, float actual) {
        if (expected != actual) {
            fail("Expected '" + expected + "', but was '" + actual + "'.");
        }
    }

    public static void assertEquals(double expected, double actual) {
        if (expected != actual) {
            fail("Expected '" + expected + "', but was '" + actual + "'.");
        }
    }

    public static void assertEquals(char expected, char actual) {
        if (expected != actual) {
            fail("Expected '" + expected + "', but was '" + actual + "'.");
        }
    }

    protected static void assertEquals(Object expected, Object actual) {
        if (expected == null && actual == null)
            return;

        if (expected != null && expected.equals(actual))
            return;

        fail("Expected '" + expected + "', but was '" + actual + "'.");
    }

    protected static void assertSame(Object expected, Object actual) {
        if (expected == actual)
            return;

        fail("Expected '" + expected + "', but was '" + actual + "'.");
    }

    protected static void assertNull(Object actual) {
        if (actual != null) {
            fail("Expected null, but was '" + actual + "'.");
        }
    }

    protected static void assertNotNull(Object actual) {
        if (actual == null) {
            fail("Expected non-null, but was '" + actual + "'.");
        }
    }

    protected static void assertTrue(boolean actual) {
        if (actual == false) {
            fail("Expected true, but was false.");
        }
    }

    protected static void assertFalse(boolean actual) {
        if (actual == true) {
            fail("Expected false, but was true.");
        }
    }

    protected static void assertClassName(String className, Object o) {
        assertTrue(o.getClass().getName().equals(className));
    }

    protected static void assertStackTraceElement(StackTraceElement e, int lineNumber, String fileName, String className, String methodName, boolean isNative) {
        assertEquals(lineNumber, e.getLineNumber());
        assertEquals(fileName, e.getFileName());
        assertEquals(className, e.getClassName());
        assertEquals(methodName, e.getMethodName());

        if (isNative)
            assertTrue(e.isNativeMethod());
        else
            assertFalse(e.isNativeMethod());
    }

    protected interface Block {
        void run() throws Throwable;
    }

    protected static void assertThrows(Block block, Class<? extends Throwable> type) {
        try {
            block.run();
        } catch (Throwable t) {
            assertEquals(type, t.getClass());
            return;
        }
        fail("Expected " + type.toString() + " to be thrown");
    }

    protected static void fail(String s) {
        throw new AssertionError(s);
    }

    protected static void fail() {
        throw new AssertionError();
    }

    public static void takeInt(int val) {}
    public static void takeLong(long val) {}
    public static void takeObject(Object obj) {}

    public static <T extends Comparable<? super T>> List<T> sort(List<T> values) {
        List<T> result = new ArrayList<T>(values);
        Collections.sort(result);
        return result;
    }

    public static <T, V> List<V> map(List<T> values, Function<T, V> f) {
        List<V> result = new ArrayList<V>();

        for (T value : values)
            result.add(f.apply(value));

        return result;
    }

    public interface Function<T, V> {
        V apply(T value);
    }
}
