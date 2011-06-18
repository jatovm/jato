package jvm;

public class MethodInvokeVirtualTest extends TestCase {
  private static final int NUM_CALLS = 1000;
  private static final int NUM_HITS = 1000;
  private static final int NUM_MISSES = 1000;

  public static void setup() {
    Orange orange = new Orange();
    Apple apple = new Apple();
    /*
     * Invoke the wrappers with the same object type in a loop to make the JIT
     * think they are monomorphic call-sites.
     */
    for (int i = 0; i < NUM_CALLS; i++) {
      orangeWrapper(orange);
      appleWrapper(apple);
    }
  }

  public static void testCallSiteWithCacheHitType() {
    for (int i = 0; i < NUM_HITS; i++) {
      assertEquals("Apple", appleWrapper(new Apple()));
      assertEquals("Orange", orangeWrapper(new Orange()));
    }
  }

  public static void testCallSiteWithCacheMissType() {
    for (int i = 0; i < NUM_MISSES; i++) {
      assertEquals("Apple", orangeWrapper(new Apple()));
      assertEquals("Orange", appleWrapper(new Orange()));
    }
  }

  public static void testCallSiteWithNull() {
    try {
      appleWrapper(null);
      fail();
    } catch (NullPointerException e) {
    }
    try {
      orangeWrapper(null);
      fail();
    } catch (NullPointerException e) {
    }
  }

  public static String appleWrapper(Fruit f) {
    return f.name();
  }

  public static String orangeWrapper(Fruit f) {
    return f.name();
  }

  public static void main(String[] args) {
    setup();
    testCallSiteWithCacheHitType();
    testCallSiteWithCacheMissType();
    testCallSiteWithNull();
  }

  public interface IFruit {
    String name();
  }

  public interface Named {
    String name();
  }

  public static abstract class Fruit implements IFruit {
  }

  public static class Apple extends Fruit implements Named {
    public String name() { return "Apple"; }
  }

  public static class Orange extends Fruit {
    public String name() { return "Orange"; }
  }
}

