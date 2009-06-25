package jato.internal;

public class VM {
  public static native void exit(int status);
  public static native void println(String line);
};
