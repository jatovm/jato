package jvm;

public class ClinitFloatTest extends TestCase {
	static class X {
		public static double x = 1.0;
	}

	public static void main(String[] args) {
		double a = 0.1;
		double b = a * X.x;

		assertEquals(b, 0.1);
	}
}
