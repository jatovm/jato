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
package test.java.lang.reflect;

import java.lang.reflect.Field;

import jvm.TestCase;

/**
 * @author Pekka Enberg
 */
public class FieldAccessorsTest extends TestCase {
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

  private static void testStaticGetShort() throws Exception {
    assertEquals(Byte.MAX_VALUE, field("staticByte").getShort(null));
    assertEquals(Short.MAX_VALUE, field("staticShort").getShort(null));
  }

  private static void testStaticGetShortFromBoolean() throws Exception {
    try {
      field("staticBoolean").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetShortFromDouble() throws Exception {
    try {
      field("staticDouble").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetShortFromFloat() throws Exception {
    try {
      field("staticFloat").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetShortFromInteger() throws Exception {
    try {
      field("staticInteger").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetShortFromChar() throws Exception {
    try {
      field("staticChar").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetShortFromLong() throws Exception {
    try {
      field("staticLong").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetShortFromObject() throws Exception {
    try {
      field("staticObject").getShort(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShort() throws Exception {
     assertEquals(Byte.MAX_VALUE, field("instanceByte").getShort(new Fields()));
     assertEquals(Short.MAX_VALUE, field("instanceShort").getShort(new Fields()));
  }

  private static void testInstanceGetShortFromBoolean() throws Exception {
    try {
      field("instanceBoolean").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShortFromDouble() throws Exception {
    try {
      field("instanceDouble").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShortFromFloat() throws Exception {
    try {
      field("instanceFloat").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShortFromInteger() throws Exception {
    try {
      field("staticInteger").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShortFromLong() throws Exception {
    try {
      field("staticLong").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShortFromChar() throws Exception {
    try {
      field("staticChar").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetShortFromObject() throws Exception {
    try {
      field("instanceObject").getShort(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetFloat() throws Exception {
    assertEquals(Byte.MAX_VALUE, field("staticByte").getFloat(null));
    assertEquals(Short.MAX_VALUE, field("staticShort").getFloat(null));
    //assertEquals(Float.MAX_VALUE, field("staticFloat").getFloat(null));
    assertEquals(Character.MAX_VALUE, field("staticChar").getFloat(null));
  }

  private static void testStaticGetFloatFromBoolean() throws Exception {
    try {
      field("staticBoolean").getFloat(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetFloatFromDouble() throws Exception {
    try {
      field("staticDouble").getFloat(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetFloatFromInteger() throws Exception {
    assertEquals((float) Integer.MAX_VALUE, field("staticInteger").getFloat(null));
  }

  private static void testStaticGetFloatFromLong() throws Exception {
    assertEquals((float) Long.MAX_VALUE, field("staticLong").getFloat(null));
  }

  private static void testStaticGetFloatFromObject() throws Exception {
    try {
      field("staticObject").getFloat(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetFloat() throws Exception {
    assertEquals(Byte.MAX_VALUE, field("instanceByte").getFloat(new Fields()));
    assertEquals(Short.MAX_VALUE, field("instanceShort").getFloat(new Fields()));
    //assertEquals(Float.MAX_VALUE, field("instanceFloat").getFloat(new Fields()));
    assertEquals(Character.MAX_VALUE, field("instanceChar").getFloat(new Fields()));
  }

  private static void testInstanceGetFloatFromBoolean() throws Exception {
    try {
      field("instanceBoolean").getFloat(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetFloatFromDouble() throws Exception {
    try {
      field("instanceDouble").getFloat(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetFloatFromInteger() throws Exception {
    assertEquals((float) Integer.MAX_VALUE, field("instanceInteger").getFloat(new Fields()));
  }

  private static void testInstanceGetFloatFromLong() throws Exception {
    assertEquals((float) Long.MAX_VALUE, field("instanceLong").getFloat(new Fields()));
  }

  private static void testInstanceGetFloatFromObject() throws Exception {
    try {
      field("instanceObject").getFloat(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetDouble() throws Exception {
    assertEquals(Byte.MAX_VALUE, field("staticByte").getDouble(null));
    assertEquals(Short.MAX_VALUE, field("staticShort").getDouble(null));
    assertEquals(Character.MAX_VALUE, field("staticChar").getDouble(null));
    //assertEquals(Float.MAX_VALUE, field("staticFloat").getDouble(null));
    //assertEquals(Double.MAX_VALUE, field("staticDouble").getDouble(null));
    assertEquals(Integer.MAX_VALUE, field("staticInteger").getDouble(null));
  }

  private static void testStaticGetDoubleFromBoolean() throws Exception {
    try {
      field("staticBoolean").getDouble(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetDoubleFromLong() throws Exception {
    assertEquals((double) Long.MAX_VALUE, field("staticLong").getDouble(null));
  }

  private static void testStaticGetDoubleFromObject() throws Exception {
    try {
      field("staticObject").getDouble(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetDouble() throws Exception {
    assertEquals(Byte.MAX_VALUE, field("instanceByte").getShort(new Fields()));
    //assertEquals(Float.MAX_VALUE, field("instanceFloat").getDouble(new Fields()));
    //assertEquals(Double.MAX_VALUE, field("instanceDouble").getDouble(new Fields()));
    assertEquals(Character.MAX_VALUE, field("instanceChar").getDouble(new Fields()));
    assertEquals(Integer.MAX_VALUE, field("instanceInteger").getDouble(new Fields()));
    assertEquals(Short.MAX_VALUE, field("instanceShort").getDouble(new Fields()));
  }

  private static void testInstanceGetDoubleFromBoolean() throws Exception {
    try {
      field("instanceBoolean").getDouble(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetDoubleFromLong() throws Exception {
    assertEquals((double) Long.MAX_VALUE, field("instanceLong").getDouble(new Fields()));
  }

  private static void testInstanceGetDoubleFromObject() throws Exception {
    try {
      field("instanceObject").getDouble(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetChar() throws Exception {
    try {
      field("staticByte").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
    try {
      field("staticShort").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
    assertEquals(Character.MAX_VALUE, field("staticChar").getChar(null));
  }

  private static void testStaticGetCharFromBoolean() throws Exception {
    try {
      field("staticBoolean").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetCharFromDouble() throws Exception {
    try {
      field("staticDouble").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetCharFromFloat() throws Exception {
    try {
      field("staticFloat").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetCharFromInteger() throws Exception {
    try {
      field("staticInteger").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetCharFromLong() throws Exception {
    try {
      field("staticLong").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetCharFromObject() throws Exception {
    try {
      field("staticObject").getChar(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetChar() throws Exception {
    try {
      field("instanceByte").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
    try {
      field("instanceShort").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
    assertEquals(Character.MAX_VALUE, field("instanceChar").getChar(new Fields()));
  }

  private static void testInstanceGetCharFromBoolean() throws Exception {
    try {
      field("instanceBoolean").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetCharFromDouble() throws Exception {
    try {
      field("instanceDouble").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetCharFromFloat() throws Exception {
    try {
      field("instanceFloat").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetCharFromInteger() throws Exception {
    try {
      field("staticInteger").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetCharFromLong() throws Exception {
    try {
      field("staticLong").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetCharFromObject() throws Exception {
    try {
      field("instanceObject").getChar(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByte() throws Exception {
     assertEquals(Byte.MAX_VALUE, field("staticByte").getByte(null));
  }

  private static void testStaticGetByteFromBoolean() throws Exception {
    try {
      field("staticBoolean").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromDouble() throws Exception {
    try {
      field("staticDouble").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromFloat() throws Exception {
    try {
      field("staticFloat").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromInteger() throws Exception {
    try {
      field("staticInteger").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromLong() throws Exception {
    try {
      field("staticLong").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromChar() throws Exception {
    try {
      field("staticChar").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromShort() throws Exception {
    try {
      field("staticShort").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetByteFromObject() throws Exception {
    try {
      field("staticObject").getByte(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByte() throws Exception {
    assertEquals(Byte.MAX_VALUE, field("instanceByte").getByte(new Fields()));
  }

  private static void testInstanceGetByteFromBoolean() throws Exception {
    try {
      field("instanceBoolean").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromDouble() throws Exception {
    try {
      field("instanceDouble").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromFloat() throws Exception {
    try {
      field("instanceFloat").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromInteger() throws Exception {
    try {
      field("instanceInteger").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromLong() throws Exception {
    try {
      field("instanceLong").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromShort() throws Exception {
    try {
      field("instanceShort").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromChar() throws Exception {
    try {
      field("instanceChar").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetByteFromObject() throws Exception {
    try {
      field("instanceObject").getByte(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBoolean() throws Exception {
     assertEquals(true, field("staticBoolean").getBoolean(null));
  }

  private static void testStaticGetBooleanFromByte() throws Exception {
    try {
      field("staticByte").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromDouble() throws Exception {
    try {
      field("staticDouble").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromFloat() throws Exception {
    try {
      field("staticFloat").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromInteger() throws Exception {
    try {
      field("staticInteger").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromLong() throws Exception {
    try {
      field("staticLong").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromChar() throws Exception {
    try {
      field("staticChar").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromShort() throws Exception {
    try {
      field("staticShort").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testStaticGetBooleanFromObject() throws Exception {
    try {
      field("staticObject").getBoolean(null);
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBoolean() throws Exception {
     assertEquals(true, field("instanceBoolean").getBoolean(new Fields()));
  }

  private static void testInstanceGetBooleanFromByte() throws Exception {
    try {
      field("instanceByte").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromDouble() throws Exception {
    try {
      field("instanceDouble").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromFloat() throws Exception {
    try {
      field("instanceFloat").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromInteger() throws Exception {
    try {
      field("instanceInteger").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromLong() throws Exception {
    try {
      field("instanceLong").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromShort() throws Exception {
    try {
      field("instanceShort").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromChar() throws Exception {
    try {
      field("instanceChar").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static void testInstanceGetBooleanFromObject() throws Exception {
    try {
      field("instanceObject").getBoolean(new Fields());
      fail("exception not thrown");
    } catch (IllegalArgumentException e) {
    }
  }

  private static Field field(String name) throws Exception {
    return Fields.class.getField(name);
  }

  private static class Fields {
    public static Object  staticObject  = new Long(Long.MAX_VALUE);
    public static boolean staticBoolean = true;
    public static byte    staticByte    = Byte.MAX_VALUE;
    public static char    staticChar    = Character.MAX_VALUE;
    public static double  staticDouble  = Long.MAX_VALUE;
    public static float   staticFloat   = Integer.MAX_VALUE;
    public static int     staticInteger	= Integer.MAX_VALUE;
    public static long    staticLong	= Long.MAX_VALUE;
    public static short   staticShort	= Short.MAX_VALUE;

    public Object  instanceObject       = new Long(Long.MAX_VALUE);
    public boolean instanceBoolean      = true;
    public byte    instanceByte         = Byte.MAX_VALUE;
    public char    instanceChar         = Character.MAX_VALUE;
    public double  instanceDouble       = Long.MAX_VALUE;
    public float   instanceFloat        = Integer.MAX_VALUE;
    public int     instanceInteger      = Integer.MAX_VALUE;
    public long    instanceLong         = Long.MAX_VALUE;
    public short   instanceShort        = Short.MAX_VALUE;
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

    testStaticGetShort();
    testStaticGetShortFromBoolean();
    testStaticGetShortFromDouble();
    testStaticGetShortFromFloat();
    testStaticGetShortFromInteger();
    testStaticGetShortFromLong();
    testStaticGetShortFromChar();
    testStaticGetShortFromObject();
    testInstanceGetShort();
    testInstanceGetShortFromBoolean();
    testInstanceGetShortFromDouble();
    testInstanceGetShortFromInteger();
    testInstanceGetShortFromLong();
    testInstanceGetShortFromChar();
    testInstanceGetShortFromFloat();
    testInstanceGetShortFromObject();

    testStaticGetFloat();
    testStaticGetFloatFromBoolean();
    testStaticGetFloatFromDouble();
    testStaticGetFloatFromInteger();
    testStaticGetFloatFromLong();
    testStaticGetFloatFromObject();
    testInstanceGetFloat();
    testInstanceGetFloatFromBoolean();
    testInstanceGetFloatFromInteger();
    testInstanceGetFloatFromDouble();
    testInstanceGetFloatFromLong();
    testInstanceGetFloatFromObject();

    testStaticGetDouble();
    testStaticGetDoubleFromBoolean();
    testStaticGetDoubleFromLong();
    testStaticGetDoubleFromObject();
    testInstanceGetDouble();
    testInstanceGetDoubleFromBoolean();
    testInstanceGetDoubleFromLong();
    testInstanceGetDoubleFromObject();

    testStaticGetChar();
    testStaticGetCharFromBoolean();
    testStaticGetCharFromDouble();
    testStaticGetCharFromFloat();
    testStaticGetCharFromInteger();
    testStaticGetCharFromLong();
    testStaticGetCharFromObject();
    testInstanceGetChar();
    testInstanceGetCharFromBoolean();
    testInstanceGetCharFromDouble();
    testInstanceGetCharFromFloat();
    testInstanceGetCharFromInteger();
    testInstanceGetCharFromLong();
    testInstanceGetCharFromObject();

    testStaticGetByte();
    testStaticGetByteFromShort();
    testStaticGetByteFromBoolean();
    testStaticGetByteFromDouble();
    testStaticGetByteFromFloat();
    testStaticGetByteFromInteger();
    testStaticGetByteFromLong();
    testStaticGetByteFromChar();
    testStaticGetByteFromObject();
    testInstanceGetByte();
    testInstanceGetByteFromShort();
    testInstanceGetByteFromBoolean();
    testInstanceGetByteFromDouble();
    testInstanceGetByteFromFloat();
    testInstanceGetByteFromInteger();
    testInstanceGetByteFromLong();
    testInstanceGetByteFromChar();
    testInstanceGetByteFromObject();

    testStaticGetBoolean();
    testStaticGetBooleanFromShort();
    testStaticGetBooleanFromByte();
    testStaticGetBooleanFromDouble();
    testStaticGetBooleanFromFloat();
    testStaticGetBooleanFromInteger();
    testStaticGetBooleanFromLong();
    testStaticGetBooleanFromChar();
    testStaticGetBooleanFromObject();
    testInstanceGetBoolean();
    testInstanceGetBooleanFromShort();
    testInstanceGetBooleanFromByte();
    testInstanceGetBooleanFromDouble();
    testInstanceGetBooleanFromFloat();
    testInstanceGetBooleanFromInteger();
    testInstanceGetBooleanFromLong();
    testInstanceGetBooleanFromChar();
    testInstanceGetBooleanFromObject();
  }
}
