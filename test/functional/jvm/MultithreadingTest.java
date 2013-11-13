/*
 * Copyright (C) 2009  Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Tomasz Grabiec
 */
public class MultithreadingTest extends TestCase {
    public static void testMainThread() {
        Thread main = Thread.currentThread();

        assertEquals("main", main.getName());
    }

    static class TestThread extends Thread {
        public boolean thread_run = false;

        public void run() {
            thread_run = true;
        }
    };

    public static void testThreadIsExecuted() {
        TestThread t;

        t = new TestThread();

        t.start();

        try {
            t.join();
        } catch (InterruptedException e) {
            fail();
        }

        assertTrue(t.thread_run);
    }

    public static void main(String []args) {
        testMainThread();
        testThreadIsExecuted();
    }
}
