/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Tomasz Grabiec
 */
public class MonitorTest extends TestCase {
    public static void testInterruptedWait() {
        Thread t = new Thread() {
            public void run() {
                boolean caught = false;

                synchronized (this) {
                    notify();
                    try {
                        wait();
                    } catch (InterruptedException e) {
                        caught = true;
                    }

                    assertTrue(caught);
                }
            }
        };

        try {
            synchronized (t) {
                t.start();
                t.wait();
                t.interrupt();
            }

            t.join();
        } catch (InterruptedException e) {
        }
    }

    public static void main(String [] args) {
        testInterruptedWait();
    }
}
