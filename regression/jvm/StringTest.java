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
