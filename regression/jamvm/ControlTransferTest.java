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

    public static void main(String [] args) {
        testGoTo();

        Runtime.getRuntime().halt(retval);
    }
}
