package test.java.lang.ref;

import java.lang.ref.SoftReference;

import jvm.TestCase;

public class ReferenceTest extends TestCase {
  public static void main(String[] args) {
    Integer ptr = new Integer(1);
    SoftReference<Integer> ref = new SoftReference<Integer>(ptr);
    assertNotNull(ref.get());
    assertEquals(ptr, ref.get());
  }
}
