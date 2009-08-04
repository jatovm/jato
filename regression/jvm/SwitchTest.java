/*
 * Copyright (C) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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

    public static void main(String []args) {
        testSwitchCaseMatches();
        testSwitchDefault();
    }
}