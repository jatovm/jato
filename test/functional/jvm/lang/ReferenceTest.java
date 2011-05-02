package jvm.lang;

import jvm.TestCase;
import java.lang.ref.SoftReference;

public class ReferenceTest extends TestCase {
  public static void main(String[] args) {
    Integer ptr = new Integer(1);
    SoftReference<Integer> ref = new SoftReference<Integer>(ptr);
    assertNotNull(ref.get());
    assertEquals(ptr, ref.get());
  }
}
