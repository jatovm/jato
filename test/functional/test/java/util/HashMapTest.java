package test.java.util;

import java.util.HashMap;

import jvm.TestCase;

public class HashMapTest extends TestCase {
  private static void testPutGet() {
    Object value = new Object();
    HashMap<String, Object> map = new HashMap<String, Object>();
    map.put("key", value);
    assertEquals(value, map.get("key"));
  }

  public static void main(String[] args) {
    testPutGet();
  }
}
