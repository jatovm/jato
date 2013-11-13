/*
 * Copyright (C) 2009 Eduard - Gabriel Munteanu <eduard.munteanu@linux360.ro>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

import jato.internal.VM;

/**
 * @author Eduard - Gabriel Munteanu
 */
public class ParameterPassingTest {
    public static int add(int a, int b, int c, int d, int e,
    			  int f, int g, int h, int i, int j) {
        return a + b + c + d + e + f + g + h + i + j;
    }

    public static void main(String[] args) {
        VM.exit(add(1, 2, 3, 4, 5, 6, 7, 8, 9, 10) +
                add(0, 1, 2, 3, 4, 5, 6, 7, 8, 9));
    }
}

