/*
 * Copyright (C) 2009 Pekka Enberg
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

import jamvm.TestCase;

public class putstatic extends TestCase {
  static class I {
    static int x, y;
    int z;
  };

  public static void testPutStaticConstInt() {
     I.x = 1;
     assertEquals(1, I.x);
  }

  public static void testPutStaticClassFieldInt() {
     I.x = 1;
     I.y = I.x;
     assertEquals(I.x, I.y);
  }

  public static void testPutStaticInstanceFieldInt() {
     I i = new I();
     i.z = 1;
     I.x = i.z;
     assertEquals(i.z, I.y);
  }

  public static void testPutStaticLocalInt() {
     int i = 1;
     I.x = i;
     assertEquals(i, I.x); 
  }

  static class J {
    static long x, y;
    long z;
  };

  public static void testPutStaticConstLong() {
     J.x = 1;
     assertEquals(1, J.x);
  }

  public static void testPutStaticClassFieldLong() {
     J.x = 1;
     J.y = J.x;
     assertEquals(J.x, J.y);
  }

  public static void testPutStaticInstanceFieldLong() {
     J j = new J();
     j.z = 1;
     J.x = j.z;
     assertEquals(j.z, J.y);
  }

  public static void testPutStaticLocalLong() {
     long j = 1;
     J.x = j;
     assertEquals(j, J.x); 
  }

  public static void main(String[] args) {
    testPutStaticConstInt();
    testPutStaticClassFieldInt();
    testPutStaticInstanceFieldInt();
    testPutStaticLocalInt();
//  testPutStaticConstLong();
//  testPutStaticClassFieldLong();
//  testPutStaticInstanceFieldLong();
//  testPutStaticLocalLong();

    Runtime.getRuntime().halt(retval);
  }
}
