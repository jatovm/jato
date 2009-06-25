package jvm;

/**
 * @author Vegard Nossum <vegard.nossum@gmail.com>
 */
public class ArrayTest extends TestCase {
	static byte[] sb = new byte[0];

	private static void testEmptyStaticArrayLength() {
		assertEquals(sb.length, 0);
	}

	byte[] b = new byte[0];

	private static void testEmptyArrayLength() {
		ArrayTest x = new ArrayTest();
		assertEquals(x.b.length, 0);
	}

	static byte[] sb2 = new byte[1];

	private static void testStaticArrayLength() {
		assertEquals(sb2.length, 1);
	}

	byte[] b2 = new byte[1];

	private static void testArrayLength() {
		ArrayTest x = new ArrayTest();
		assertEquals(x.b2.length, 1);
	}

	public static void main(String args[]) {
		testEmptyStaticArrayLength();
		testEmptyArrayLength();

		testStaticArrayLength();
		testArrayLength();

		exit();
	}
}
