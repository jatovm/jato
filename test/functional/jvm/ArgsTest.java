/*
 * Copyright (C) 2009 Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */
package jvm;

/**
 * @author Pekka Enberg
 */
public class ArgsTest extends TestCase {
    public static void main(String[] args) {
        assertNotNull(args);
    }
}
