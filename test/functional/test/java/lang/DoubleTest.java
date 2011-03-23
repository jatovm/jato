package test.java.lang;

import jvm.TestCase;

public class DoubleTest extends TestCase {
  public static void testToString() {
    assertEquals("1.0", Double.toString(1.0));
    assertEquals("1.1", Double.toString(1.1));
  }

  public static void main(String[] args) {
    testToString();
  }
}
