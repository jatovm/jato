package java.lang;

public class VMSystem {
    static native void arraycopy(Object src, int srcStart, Object dest, int destStart, int len);
    static native int identityHashCode(Object o);
}
