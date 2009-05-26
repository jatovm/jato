package jvm;

/**
 * @author Vegard Nossum <vegard.nossum@gmail.com>
 */
public class StringTest extends TestCase {
	public static void main(String args[]) {
		String s = "æøå";
		assertEquals(s.length(), 3);

		String t = "ヒラガナ";
		assertEquals(t.length(), 4);

		exit(retval);
	}
}
