/*
 * Copyright (C) 2012 Eduard - Gabriel Munteanu <eduard.munteanu@linux360.ro>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

import jato.internal.VM;

/**
 * @author Eduard - Gabriel Munteanu
 */
public class ParameterPassingLivenessTest extends TestCase {
    private static int run(int[] a, int v) {
        return v;
    }

    public static void main(String[] args) {
        int n = run(new int[8], 1);
        VM.exit(n);
    }
}
