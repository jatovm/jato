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
    private static void testStaticGetObject() throws Exception {
       assertEquals(Fields.staticBoolean, field("staticBoolean").get(null));
       assertEquals(Fields.staticByte   , field("staticByte").get(null));
       assertEquals(Fields.staticChar   , field("staticChar").get(null));
       assertEquals(Fields.staticDouble , field("staticDouble").get(null));
       assertEquals(Fields.staticFloat  , field("staticFloat").get(null));
       assertEquals(Fields.staticInteger, field("staticInteger").get(null));
       assertEquals(Fields.staticLong   , field("staticLong").get(null));
       assertEquals(Fields.staticObject , field("staticObject").get(null));
       assertEquals(Fields.staticShort  , field("staticShort").get(null));
       assertEquals(Fields.staticShort  , field("staticShort").get(null));
    }

    private static void testInstanceGetObject() throws Exception {
       Fields fields = new Fields();
       assertEquals(fields.instanceBoolean, field("instanceBoolean").get(fields));
       assertEquals(fields.instanceByte   , field("instanceByte").get(fields));
       assertEquals(fields.instanceChar   , field("instanceChar").get(fields));
       assertEquals(fields.instanceDouble , field("instanceDouble").get(fields));
       assertEquals(fields.instanceFloat  , field("instanceFloat").get(fields));
       assertEquals(fields.instanceInteger, field("instanceInteger").get(fields));
       assertEquals(fields.instanceLong   , field("instanceLong").get(fields));
       assertEquals(fields.instanceObject , field("instanceObject").get(fields));
       assertEquals(fields.instanceShort  , field("instanceShort").get(fields));
    }

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
       assertEquals(Byte.MAX_VALUE, field("instanceByte").getLong(new Fields()));
       assertEquals(Character.MAX_VALUE, field("instanceChar").getLong(new Fields()));
       assertEquals(Integer.MAX_VALUE, field("instanceInteger").getLong(new Fields()));
       assertEquals(Long.MAX_VALUE, field("instanceLong").getLong(new Fields()));
       assertEquals(Short.MAX_VALUE, field("instanceShort").getLong(new Fields()));
    }

    private static void testInstanceGetLongFromBoolean() throws Exception {
      try {
        field("instanceBoolean").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLongFromDouble() throws Exception {
      try {
        field("instanceDouble").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLongFromFloat() throws Exception {
      try {
        field("instanceFloat").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static void testInstanceGetLongFromObject() throws Exception {
      try {
        field("instanceObject").getLong(new Fields());
        fail("exception not thrown");
      } catch (IllegalArgumentException e) {
      }
    }

    private static Field field(String name) throws Exception {
        return Fields.class.getField(name);
    }

    private static class Fields {
        public static Object  staticObject	= new Long(Long.MAX_VALUE);
        public static boolean staticBoolean	= true;
        public static byte    staticByte	= Byte.MAX_VALUE;
        public static char    staticChar	= Character.MAX_VALUE;
        public static double  staticDouble	= Long.MAX_VALUE;
        public static float   staticFloat	= Integer.MAX_VALUE;
        public static int     staticInteger	= Integer.MAX_VALUE;
        public static long    staticLong	= Long.MAX_VALUE;
        public static short   staticShort	= Short.MAX_VALUE;

        public Object  instanceObject	= new Long(Long.MAX_VALUE);
        public boolean instanceBoolean	= true;
        public byte    instanceByte		= Byte.MAX_VALUE;
        public char    instanceChar		= Character.MAX_VALUE;
        public double  instanceDouble	= Long.MAX_VALUE;
        public float   instanceFloat	= Integer.MAX_VALUE;
        public int     instanceInteger	= Integer.MAX_VALUE;
        public long    instanceLong		= Long.MAX_VALUE;
        public short   instanceShort	= Short.MAX_VALUE;
    };

    public static void main(String[] args) throws Exception {
      testStaticGetObject();
      testInstanceGetObject();
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
