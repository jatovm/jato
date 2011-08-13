#ifndef JATO__PPC_ENCODE_H
#define JATO__PPC_ENCODE_H

struct basic_block;
struct buffer;
struct insn;

#define OPCD(x)         (x << 26)
#define LI(x)           (x <<  2)
#define AA(x)           (x <<  1)
#define LK(x)           (x <<  0)

#define BO(x)           (x << 21)
#define BI(x)           (x << 16)

#define S(x)		(x << 21)
#define CRM(x)		(x << 12)

#define SPR(x)		(x << 16)

#define D(x)		(x << 21)
#define A(x)		(x << 16)

#define SIMM(x)		(x <<  0)
#define UIMM(x)		(x <<  0)

enum {
	BO_BR_ALWAYS	= 20,	/* branch always */
};

enum {
	SPR_XER		= 1,
	SPR_LR		= 8,
	SPR_CTR		= 9,
};

static inline unsigned long bl(unsigned long li)
{
        return OPCD(18) | LI(li) | AA(0) | LK(1);
}

/* Branch Conditional to Link Register */
static inline unsigned long bclr(unsigned char bo, unsigned char bi, unsigned char lk)
{
	return OPCD(19) | BO(bo) | BI(bi) | 16 << 1 | LK(lk);
}

static inline unsigned long blr(void)
{
	return bclr(BO_BR_ALWAYS, 0, 0);
}

/* Branch Conditional To Count Register */
static inline unsigned long bcctr(unsigned char bo, unsigned char bi, unsigned char lk)
{
	return OPCD(19) | BO(bo) | BI(bi) | (528 << 1) | LK(lk);
}

static inline unsigned long bctr(void)
{
	return bcctr(BO_BR_ALWAYS, 0, 0);
}

static inline unsigned long bctrl(void)
{
	return bcctr(BO_BR_ALWAYS, 0, 1);
}

/* Move from Special-Purpose Register */
static inline unsigned long mfspr(unsigned char rd, unsigned char spr)
{
	return OPCD(31) | D(rd) | SPR(spr) | (339 << 1);
}

static inline unsigned long mflr(unsigned long rd)
{
	return mfspr(rd, SPR_LR);
}

/* Move to Special-Purpose Register */
static inline unsigned long mtspr(unsigned char spr, unsigned char rs)
{
	return OPCD(31) | S(rs) | SPR(spr) | (467 << 1);
}

static inline unsigned long mtctr(unsigned long rs)
{
	return mtspr(SPR_CTR, rs);
}

static inline unsigned long mtlr(unsigned long rs)
{
	return mtspr(SPR_LR, rs);
}

/* Add Immediate */
static inline unsigned long addi(unsigned char rd, unsigned char ra, unsigned short simm)
{
	return OPCD(14) | D(rd) | A(ra) | SIMM(simm);
}

/* Add Immediate Shifted */
static inline unsigned long addis(unsigned char rd, unsigned char ra, unsigned short simm)
{
	return OPCD(15) | D(rd) | A(ra) | SIMM(simm);
}

static inline unsigned long lis(unsigned char rd, unsigned short value)
{
	return addis(rd, 0, value);
}

static inline unsigned long lwz(unsigned char rd, unsigned short d, unsigned char ra)
{
	return OPCD(32) | D(rd) | A(ra) | d;
}

/* OR Immediate */
static inline unsigned long ori(unsigned char ra, unsigned char rs, unsigned short uimm)
{
	return OPCD(24) | S(rs) | A(ra) | UIMM(uimm);
}

/* Store Word */
static inline unsigned long stw(unsigned char rs, unsigned short d, unsigned char ra)
{
	return OPCD(37) | S(rs) | A(ra) | d;
}

/* Store Word with Update */
static inline unsigned long stwu(unsigned char rs, unsigned short d, unsigned char ra)
{
	return OPCD(37) | S(rs) | A(ra) | d;
}

static inline unsigned short ptr_high(void *p)
{
	unsigned long x = (unsigned long) p;

	return x >> 16;
}

static inline unsigned short ptr_low(void *p)
{
	unsigned long x = (unsigned long) p;

	return x & 0xffff;
}

void insn_encode(struct insn *self, struct buffer *buffer, struct basic_block *bb);

#endif /* JATO__PPC_ENCODE_H */
