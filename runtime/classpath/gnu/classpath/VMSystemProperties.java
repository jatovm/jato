package gnu.classpath;

import java.util.Properties;

public class VMSystemProperties {
    static native void preInit(Properties p);

    static void postInit(Properties p) {
    }
}
