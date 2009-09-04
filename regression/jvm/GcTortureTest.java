package jvm;

public class GcTortureTest {
    public static void main(String[] args) throws Exception {
        Thread[] threads = new Thread[8];

        for (int i = 0; i < threads.length; i++) {
            threads[i] = new Thread(new Runnable() {
                public void run() {
                    for (int i = 0; i < 1000000; i++)
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
