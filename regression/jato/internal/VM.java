package jato.internal;

public class VM {
  public static final int FAULT_IN_CLASS_INIT = 1;

  public static native void exit(int status);
  public static native void println(String line);
  public static native void enableFault(int kind, Object arg);
  public static native void disableFault(int kind);
  public static native void throwNullPointerException();
};
