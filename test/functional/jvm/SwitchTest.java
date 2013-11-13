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
class SwitchTest extends TestCase {
    public static void testSwitchCaseMatches() {
        int index;

        index = 2;

        switch (index) {
        case 1:
            index = -1;
            break;
        case 2:
            index = -2;
            break;
        case 3:
            index = -3;
            break;
        }

        assertEquals(-2, index);
    }

    public static void testSwitchDefault() {
        int index;

        index = 0;

        switch (index) {
        case 1:
            index = -1;
            break;
        case 2:
            index = -2;
            break;
        case 3:
            index = -3;
            break;
        default:
            index = -4;
        }

        assertEquals(-4, index);
    }

    public static void testLookupswitchCaseMatches() {
        int index;

        index = 4000;

        switch (index) {
        case -100:
            index = -1;
            break;
        case 2000:
            index = -2;
            break;
        case 4000:
            index = -3;
            break;
        case 50000:
            index = -4;
            break;
        case 221:
            index = -5;
            break;
        case -34:
            index = -6;
            break;
        default:
            index = -7;
        }

        assertEquals(-3, index);
    }

    public static void testLookupswitchDefault() {
        int index;

        index = 314;

        switch (index) {
        case -100:
            index = -1;
            break;
        case 2000:
            index = -2;
            break;
        case 4000:
            index = -3;
            break;
        case 50000:
            index = -4;
            break;
        case 221:
            index = -5;
            break;
        case -34:
            index = -6;
            break;
        default:
            index = -7;
        }

        assertEquals(-7, index);
    }

    public static void main(String []args) {
        testSwitchCaseMatches();
        testSwitchDefault();
        testLookupswitchCaseMatches();
        testLookupswitchDefault();
    }
}