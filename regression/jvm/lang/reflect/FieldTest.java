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
package jvm.lang.reflect;

import java.lang.reflect.Field;
import jvm.TestCase;

/**
 * @author Pekka Enberg
 */
public class FieldTest extends TestCase {
    private static void testStaticGetLong() throws Exception {
       assertEquals(Byte.MAX_VALUE, field("staticByte").getLong(null));
       assertEquals(Character.MAX_VALUE, field("staticChar").getLong(null));
       assertEquals(Integer.MAX_VALUE, field("staticInteger").getLong(null));
       assertEquals(Long.MAX_VALUE, field("staticLong").getLong(null));
       assertEquals(Short.MAX_VALUE, field("staticShort").getLong(null));
    }

    private static void testStaticGetLongFromBoolean() throws Exception {
      try {
        field("staticBoolean").getLong(null);
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testStaticGetLongFromDouble() throws Exception {
      try {
        field("staticDouble").getLong(null);
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testStaticGetLongFromFloat() throws Exception {
      try {
        field("staticFloat").getLong(null);
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testStaticGetLongFromObject() throws Exception {
      try {
        field("staticObject").getLong(null);
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLong() throws Exception {
       assertEquals(Byte.MAX_VALUE, field("staticByte").getLong(new Fields()));
       assertEquals(Character.MAX_VALUE, field("staticChar").getLong(new Fields()));
       assertEquals(Integer.MAX_VALUE, field("staticInteger").getLong(new Fields()));
       assertEquals(Long.MAX_VALUE, field("staticLong").getLong(new Fields()));
       assertEquals(Short.MAX_VALUE, field("staticShort").getLong(new Fields()));
    }

    private static void testInstanceGetLongFromBoolean() throws Exception {
      try {
        field("staticBoolean").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLongFromDouble() throws Exception {
      try {
        field("staticDouble").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLongFromFloat() throws Exception {
      try {
        field("staticFloat").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLongFromObject() throws Exception {
      try {
        field("staticObject").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static Field field(String name) throws Exception {
        return Fields.class.getField(name);
    }

    private static class Fields {
        @SuppressWarnings("unused") public static Object  staticObject	= new Long(Long.MAX_VALUE);
        @SuppressWarnings("unused") public static boolean staticBoolean	= true;
        @SuppressWarnings("unused") public static byte    staticByte	= Byte.MAX_VALUE;
        @SuppressWarnings("unused") public static char    staticChar	= Character.MAX_VALUE;
        @SuppressWarnings("unused") public static double  staticDouble	= Long.MAX_VALUE;
        @SuppressWarnings("unused") public static float   staticFloat	= Integer.MAX_VALUE;
        @SuppressWarnings("unused") public static int     staticInteger	= Integer.MAX_VALUE;
        @SuppressWarnings("unused") public static long    staticLong	= Long.MAX_VALUE;
        @SuppressWarnings("unused") public static short   staticShort	= Short.MAX_VALUE;

        @SuppressWarnings("unused") public Object  instanceObject	= new Long(Long.MAX_VALUE);
        @SuppressWarnings("unused") public boolean instanceBoolean	= true;
        @SuppressWarnings("unused") public byte    instanceByte		= Byte.MAX_VALUE;
        @SuppressWarnings("unused") public char    instanceChar		= Character.MAX_VALUE;
        @SuppressWarnings("unused") public double  instanceDouble	= Long.MAX_VALUE;
        @SuppressWarnings("unused") public float   instanceFloat	= Integer.MAX_VALUE;
        @SuppressWarnings("unused") public int     instanceInteger	= Integer.MAX_VALUE;
        @SuppressWarnings("unused") public long    instanceLong		= Long.MAX_VALUE;
        @SuppressWarnings("unused") public short   instanceShort	= Short.MAX_VALUE;
    };

    public static void main(String[] args) throws Exception {
      testStaticGetLong();
      testStaticGetLongFromBoolean();
      testStaticGetLongFromDouble();
      testStaticGetLongFromFloat();
      testStaticGetLongFromObject();
      testInstanceGetLong();
      testInstanceGetLongFromBoolean();
      testInstanceGetLongFromDouble();
      testInstanceGetLongFromFloat();
      testInstanceGetLongFromObject();
    }
}
