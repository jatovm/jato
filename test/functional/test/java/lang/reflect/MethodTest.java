package test.java.lang.reflect;

import jvm.TestCase;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

public class MethodTest extends TestCase {
  public static void testGetAnnotation() throws Exception {
    Method m = TaggedClass.class.getMethod("foo");
    Field f = TaggedClass.class.getField("myField");
    Tag tag = m.getAnnotation(Tag.class);
    Tag ftag = f.getAnnotation(Tag.class);
    assertNotNull(tag);
    assertNotNull(ftag);

    assertEquals(Byte.MAX_VALUE, tag.byteValue());
    assertEquals(Character.MAX_VALUE, tag.charValue());
    assertEquals(Short.MAX_VALUE, tag.shortValue());
    assertEquals(Integer.MAX_VALUE, tag.intValue());
    assertEquals(Long.MAX_VALUE, tag.longValue());
    assertEquals(Float.MAX_VALUE, tag.floatValue());
    assertEquals(Double.MAX_VALUE, tag.doubleValue());
    assertEquals("hello, world", tag.stringValue());
    assertEquals(Required.YES, tag.enumValue());
    assertEquals(Object.class, tag.classValue());
    assertNotNull(tag.annotationValue());

    assertArrayEquals(new byte[]     { Byte.MIN_VALUE,      Byte.MAX_VALUE      }, tag.byteArrayValue());
    assertArrayEquals(new char[]     { Character.MIN_VALUE, Character.MAX_VALUE }, tag.charArrayValue());
    assertArrayEquals(new short[]    { Short.MIN_VALUE,     Short.MAX_VALUE     }, tag.shortArrayValue());
    assertArrayEquals(new int[]      { Integer.MIN_VALUE,   Integer.MAX_VALUE   }, tag.intArrayValue());
    assertArrayEquals(new long[]     { Long.MIN_VALUE,      Long.MAX_VALUE      }, tag.longArrayValue());
    assertArrayEquals(new float[]    { Float.MIN_VALUE,     Float.MAX_VALUE     }, tag.floatArrayValue());
    assertArrayEquals(new double[]   { Double.MIN_VALUE,    Double.MAX_VALUE    }, tag.doubleArrayValue());
    assertArrayEquals(new String[]   { "hello, world",      "Hello, World!"     }, tag.stringArrayValue());
    assertArrayEquals(new Required[] { Required.YES,        Required.NO         }, tag.enumArrayValue());
    assertArrayEquals(new Class<?>[] { Integer.class,       Long.class          }, tag.classArrayValue());
    assertNotNull(tag.annotationArrayValue());

    assertEquals(Byte.MAX_VALUE, ftag.byteValue());
    assertEquals(Character.MAX_VALUE, ftag.charValue());
    assertEquals(Short.MAX_VALUE, ftag.shortValue());
    assertEquals(Integer.MAX_VALUE, ftag.intValue());
    assertEquals(Long.MAX_VALUE, ftag.longValue());
    assertEquals(Float.MAX_VALUE, ftag.floatValue());
    assertEquals(Double.MAX_VALUE, ftag.doubleValue());
    assertEquals("hello, world", ftag.stringValue());
    assertEquals(Required.YES, ftag.enumValue());
    assertEquals(Object.class, ftag.classValue());
    assertNotNull(ftag.annotationValue());

    assertArrayEquals(new byte[]     { Byte.MIN_VALUE,      Byte.MAX_VALUE      }, ftag.byteArrayValue());
    assertArrayEquals(new char[]     { Character.MIN_VALUE, Character.MAX_VALUE }, ftag.charArrayValue());
    assertArrayEquals(new short[]    { Short.MIN_VALUE,     Short.MAX_VALUE     }, ftag.shortArrayValue());
    assertArrayEquals(new int[]      { Integer.MIN_VALUE,   Integer.MAX_VALUE   }, ftag.intArrayValue());
    assertArrayEquals(new long[]     { Long.MIN_VALUE,      Long.MAX_VALUE      }, ftag.longArrayValue());
    assertArrayEquals(new float[]    { Float.MIN_VALUE,     Float.MAX_VALUE     }, ftag.floatArrayValue());
    assertArrayEquals(new double[]   { Double.MIN_VALUE,    Double.MAX_VALUE    }, ftag.doubleArrayValue());
    assertArrayEquals(new String[]   { "hello, world",      "Hello, World!"     }, ftag.stringArrayValue());
    assertArrayEquals(new Required[] { Required.YES,        Required.NO         }, ftag.enumArrayValue());
    assertArrayEquals(new Class<?>[] { Integer.class,       Long.class          }, ftag.classArrayValue());
    assertNotNull(ftag.annotationArrayValue());
  }

  public static class TaggedClass {
    @Tag(
      byteValue        = Byte.MAX_VALUE,
      charValue        = Character.MAX_VALUE,
      shortValue       = Short.MAX_VALUE,
      intValue         = Integer.MAX_VALUE,
      longValue        = Long.MAX_VALUE,
      floatValue       = Float.MAX_VALUE,
      doubleValue      = Double.MAX_VALUE,
      stringValue      = "hello, world",
      enumValue        = Required.YES,
      classValue       = Object.class,
      annotationValue  = @Tag2,

      byteArrayValue   = { Byte.MIN_VALUE,      Byte.MAX_VALUE      },
      charArrayValue   = { Character.MIN_VALUE, Character.MAX_VALUE },
      shortArrayValue  = { Short.MIN_VALUE,     Short.MAX_VALUE     },
      intArrayValue    = { Integer.MIN_VALUE,   Integer.MAX_VALUE   },
      longArrayValue   = { Long.MIN_VALUE,      Long.MAX_VALUE      },
      floatArrayValue  = { Float.MIN_VALUE,     Float.MAX_VALUE     },
      doubleArrayValue = { Double.MIN_VALUE,    Double.MAX_VALUE    },
      stringArrayValue = { "hello, world",      "Hello, World!"     },
      enumArrayValue   = { Required.YES,        Required.NO         },
      classArrayValue  = { Integer.class,       Long.class          },
      annotationArrayValue = { @Tag2, @Tag2 }
    )
    public void foo() { }

    @Tag(
      byteValue        = Byte.MAX_VALUE,
      charValue        = Character.MAX_VALUE,
      shortValue       = Short.MAX_VALUE,
      intValue         = Integer.MAX_VALUE,
      longValue        = Long.MAX_VALUE,
      floatValue       = Float.MAX_VALUE,
      doubleValue      = Double.MAX_VALUE,
      stringValue      = "hello, world",
      enumValue        = Required.YES,
      classValue       = Object.class,
      annotationValue  = @Tag2,

      byteArrayValue   = { Byte.MIN_VALUE,      Byte.MAX_VALUE      },
      charArrayValue   = { Character.MIN_VALUE, Character.MAX_VALUE },
      shortArrayValue  = { Short.MIN_VALUE,     Short.MAX_VALUE     },
      intArrayValue    = { Integer.MIN_VALUE,   Integer.MAX_VALUE   },
      longArrayValue   = { Long.MIN_VALUE,      Long.MAX_VALUE      },
      floatArrayValue  = { Float.MIN_VALUE,     Float.MAX_VALUE     },
      doubleArrayValue = { Double.MIN_VALUE,    Double.MAX_VALUE    },
      stringArrayValue = { "hello, world",      "Hello, World!"     },
      enumArrayValue   = { Required.YES,        Required.NO         },
      classArrayValue  = { Integer.class,       Long.class          },
      annotationArrayValue = { @Tag2, @Tag2 }
    )
    public String myField = null;
  }

  @Retention(RetentionPolicy.RUNTIME)
  public @interface Tag {
    byte       byteValue();
    char       charValue();
    short      shortValue();
    int        intValue();
    long       longValue();
    float      floatValue();
    double     doubleValue();
    String     stringValue();
    Required   enumValue();
    Class<?>   classValue();
    Tag2       annotationValue();

    byte[]     byteArrayValue();
    char[]     charArrayValue();
    short[]    shortArrayValue();
    int[]      intArrayValue();
    long[]     longArrayValue();
    float[]    floatArrayValue();
    double[]   doubleArrayValue();
    String[]   stringArrayValue();
    Required[] enumArrayValue();
    Class<?>[] classArrayValue();
    Tag2[]     annotationArrayValue();
  }

  public static enum Required { YES, NO }

  public @interface Tag2 { }

  public static void main(String[] args) throws Exception {
    testGetAnnotation();
  }
}
