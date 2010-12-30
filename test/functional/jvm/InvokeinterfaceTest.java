package jvm;

/**
 * The purpose of this test is to see whether our itable approach is working
 * like it should. With this many methods, we are certain to exhaust the
 * itable capacity and generate conflict resolution stubs. We check that the
 * right methods have been called by setting a flag in only four methods out
 * of all, and we check that these four flags have been set.
 */
public class InvokeinterfaceTest extends TestCase {
    private interface A {
        public void aa();
        public void ab();
        public void ac();
        public void ad();
        public void ae();
        public void af();
        public void ag();
        public void ah();
        public void ai();
        public void aj();
        public void ak();
        public void al();
        public void am();
        public void an();
        public void ap();
        public void aq();
    }

    private interface B {
        public void ba();
        public void bb();
        public void bc();
        public void bd();
        public void be();
        public void bf();
        public void bg();
        public void bh();
        public void bi();
        public void bj();
        public void bk();
        public void bl();
        public void bm();
        public void bn();
        public void bp();
        public void bq();
    }

    private interface C {
        public void ca();
        public void cb();
        public void cc();
        public void cd();
        public void ce();
        public void cf();
        public void cg();
        public void ch();
        public void ci();
        public void cj();
        public void ck();
        public void cl();
        public void cm();
        public void cn();
        public void cp();
        public void cq();
    }

    private interface D {
        public void da();
        public void db();
        public void dc();
        public void dd();
        public void de();
        public void df();
        public void dg();
        public void dh();
        public void di();
        public void dj();
        public void dk();
        public void dl();
        public void dm();
        public void dn();
        public void dp();
        public void dq();
    }

    private static class X implements A, B, C, D {
        public void aa() {}
        public void ab() {}
        public void ac() {}
        public void ad() {}
        public void ae() {}
        public void af() {}
        public void ag() { a_ok = true; }
        public void ah() {}
        public void ai() {}
        public void aj() {}
        public void ak() {}
        public void al() {}
        public void am() {}
        public void an() {}
        public void ap() {}
        public void aq() {}
        public void ba() {}
        public void bb() {}
        public void bc() {}
        public void bd() {}
        public void be() {}
        public void bf() {}
        public void bg() {}
        public void bh() {}
        public void bi() {}
        public void bj() {}
        public void bk() {}
        public void bl() {}
        public void bm() {}
        public void bn() {}
        public void bp() { b_ok = true; }
        public void bq() {}
        public void ca() {}
        public void cb() {}
        public void cc() {}
        public void cd() {}
        public void ce() {}
        public void cf() {}
        public void cg() {}
        public void ch() {}
        public void ci() { c_ok = true; }
        public void cj() {}
        public void ck() {}
        public void cl() {}
        public void cm() {}
        public void cn() {}
        public void cp() {}
        public void cq() {}
        public void da() {}
        public void db() {}
        public void dc() {}
        public void dd() {}
        public void de() {}
        public void df() {}
        public void dg() {}
        public void dh() {}
        public void di() {}
        public void dj() {}
        public void dk() {}
        public void dl() {}
        public void dm() {}
        public void dn() {}
        public void dp() {}
        public void dq() { d_ok = true; }
    }

    private static boolean a_ok, b_ok, c_ok, d_ok;

    private static void testInvokeinterface() {
        X x = new X();

        A a = (A) x;
        a.ag();
        assertTrue(a_ok);

        B b = (B) x;
        b.bp();
        assertTrue(b_ok);

        C c = (C) x;
        c.ci();
        assertTrue(c_ok);

        D d = (D) x;
        d.dq();
        assertTrue(d_ok);
    }

    private static void testInvokeinterfaceItableHashCollision() {
        ISeq iseq = (ISeq) new Cons();
        iseq.seq();
    }

    private static final class Cons implements ISeq {
        public void j() {
            throw new AssertionError("wrong method");
        }

        public void seq() {
        }
    }

    private static interface ISeq extends Seqable {
        public void j();
    }

    private static interface Seqable {
        public void seq();
    }

    public static void main(String[] args) {
        testInvokeinterface();
        testInvokeinterfaceItableHashCollision();
    }
}
