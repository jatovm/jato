/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

import jato.internal.VM;

/**
 * @author Pekka Enberg
 */
public class ExitStatusIsOneTest {
    public static void main(String[] args) {
        VM.exit(1);
    }
}
