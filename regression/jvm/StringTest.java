package jvm;

/**
 * @author Vegard Nossum
 */
public class StringTest extends TestCase {
    public static void testUnicode() {
        String s = "æøå";
        assertEquals(s.length(), 3);

        String t = "ヒラガナ";
        assertEquals(t.length(), 4);

        String p = "\u1234\u5678\uabcd";
        assertEquals(0x1234, (int)p.charAt(0));
        assertEquals(0x5678, (int)p.charAt(1));
        assertEquals(0xabcd, (int)p.charAt(2));
    }

    public static void testStringConcatenation() {
        String a = "123";
        String b = "abcd";

        assertObjectEquals("123abcd", a + b);
    }

    public static void main(String args[]) {
        testUnicode();
        testStringConcatenation();
    }
}
