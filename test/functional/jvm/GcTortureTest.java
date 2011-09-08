package jvm;

public class GcTortureTest {
    public static void main(String[] args) throws Exception {
        Thread[] threads = new Thread[32];

        /* One thread doesn't do any GC.  */
        threads[0] = new Thread(new Runnable() {
            int result;

            public void run() {
                for (int i = 0; i < 100000; i++)
                    result += i;
            }
        });

        for (int i = 1; i < threads.length; i++) {
            threads[i] = new Thread(new Runnable() {
                public void run() {
                    for (int i = 0; i < 10000; i++)
                        new Object();
                }
            });
        }

        for (int i = 0; i < threads.length; i++)
            threads[i].start();

        for (int i = 0; i < threads.length; i++)
            threads[i].join();
    }
}
