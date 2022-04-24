/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 *	@(#)tty_subr.c	1.1 (2.10BSD Berkeley) 12/1/86
 */

#include <sys/cdefs.h>
#include <sys/param.h>
#include <sys/clist.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/map.h>
#include <sys/malloc.h>
//#include <sys/user.h>

struct cblock *cfreelist = 0;
int cfreecount = 0;
static int 	ctotcount;
char		cwaiting;

static void cblock_alloc_cblocks(int);
static void cblock_free_cblocks(int);

/*
 * Initialize clist by freeing all character blocks, then count
 * number of character devices. (Once-only routine)
 */
void
cinit(void)
{
	register int ccp;
	register struct cblock *cp;

	ccp = (int) cfree;
	ccp = (ccp + CROUND) & ~CROUND;

	for (cp = (struct cblock *)ccp; cp <= &cfree[nclist - 1]; cp++) {
		cp->c_next = cfreelist;
		cfreelist = cp;
		cfreecount += CBSIZE;
	}
}

/*
 * Remove a cblock from the cfreelist queue and return a pointer
 * to it.
 */
struct cblock *
cblock_alloc()
{
	struct cblock *cblockp;

	cblockp = cfreelist;
	if (cblockp == NULL)
		panic("clist reservation botch");
	cfreelist = cblockp->c_next;
	cblockp->c_next = NULL;
	cfreecount -= CBSIZE;
	return (cblockp);
}

/*
 * Add a cblock to the cfreelist queue.
 */
void
cblock_free(cblockp)
	struct cblock *cblockp;
{
	cblockp->c_next = cfreelist;
	cfreelist = cblockp;
	cfreecount += CBSIZE;
}

/*
 * Allocate some cblocks for the cfreelist queue.
 */
static void
cblock_alloc_cblocks(number)
	int number;
{
	int i;
	struct cblock *tmp;

	for (i = 0; i < number; ++i) {
		tmp = malloc(sizeof(struct cblock), M_TTY, M_WAITOK);
		bzero((char *)tmp, sizeof(struct cblock));
		cblock_free(tmp);
	}
	ctotcount += number;
}

/*
 * Set the cblock allocation policy for a a clist.
 * Must be called at spltty().
 */
void
clist_alloc_cblocks(clistp, ccmax, ccreserved)
	struct clist *clistp;
	int ccmax;
	int ccreserved;
{
	int dcbr;

	clistp->c_cbmax = roundup(ccmax, CBSIZE) / CBSIZE;
	dcbr = roundup(ccreserved, CBSIZE) / CBSIZE - clistp->c_cbreserved;
	if (dcbr >= 0)
		cblock_alloc_cblocks(dcbr);
	else {
		if (clistp->c_cbreserved + dcbr < clistp->c_cbcount)
			dcbr = clistp->c_cbcount - clistp->c_cbreserved;
		cblock_free_cblocks(-dcbr);
	}
	clistp->c_cbreserved += dcbr;
}

/*
 * Free some cblocks from the cfreelist queue back to the
 * system malloc pool.
 */
static void
cblock_free_cblocks(number)
	int number;
{
	int i;

	for (i = 0; i < number; ++i)
		free(cblock_alloc(), M_TTY);
	ctotcount -= number;
}

/*
 * Free the cblocks reserved for a clist.
 * Must be called at spltty().
 */
void
clist_free_cblocks(clistp)
	struct clist *clistp;
{
	if (clistp->c_cbcount != 0)
		panic("freeing active clist cblocks");
	cblock_free_cblocks(clistp->c_cbreserved);
	clistp->c_cbmax = 0;
	clistp->c_cbreserved = 0;
}

/*
 * Character list get/put
 */
int
getc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;

	s = spltty();
	if (p->c_cc <= 0) {
		c = -1;
		p->c_cc = 0;
		p->c_cf = p->c_cl = NULL;
	} else {
		c = *p->c_cf++ & 0377;
		if (--p->c_cc<=0) {
			bp = (struct cblock *)(p->c_cf-1);
			bp = (struct cblock *)((int)bp & ~CROUND);
			p->c_cf = NULL;
			p->c_cl = NULL;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
		} else if (((int)p->c_cf & CROUND) == 0){
			bp = (struct cblock *)(p->c_cf);
			bp--;
			p->c_cf = bp->c_next->c_info;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
		}
	}
	splx(s);
	return (c);
}

/*
 * copy clist to buffer.
 * return number of bytes moved.
 */
int
q_to_b(q, cp, cc)
	register struct clist *q;
	char *cp;
	int cc;
{
	register struct cblock *bp;
	register int nc;
	int s;
	char *acp;

	if (cc <= 0)
		return (0);
	s = spltty();

	if (q->c_cc <= 0) {
		q->c_cc = 0;
		q->c_cf = q->c_cl = NULL;
		splx(s);
		return (0);
	}
	acp = cp;

	while (cc) {
		nc = sizeof (struct cblock) - ((int)q->c_cf & CROUND);
		nc = MIN(nc, cc);
		nc = MIN(nc, q->c_cc);
		(void) bcopy(q->c_cf, cp, (unsigned)nc);
		q->c_cf += nc;
		q->c_cc -= nc;
		cc -= nc;
		cp += nc;
		if (q->c_cc <= 0) {
			bp = (struct cblock *)(q->c_cf - 1);
			bp = (struct cblock *)((int)bp & ~CROUND);
			q->c_cf = q->c_cl = NULL;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
			break;
		}
		if (((int)q->c_cf & CROUND) == 0) {
			bp = (struct cblock *)(q->c_cf);
			bp--;
			q->c_cf = bp->c_next->c_info;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
		}
	}
	splx(s);
	return (cp-acp);
}

/*
 * Return count of contiguous characters
 * in clist starting at q->c_cf.
 * Stop counting if flag&character is non-null.
 */
int
ndqb(q, flag)
	register struct clist *q;
	int flag;
{
	int cc;
	int s;

	s = spltty();
	if (q->c_cc <= 0) {
		cc = -q->c_cc;
		goto out;
	}
	cc = ((int)q->c_cf + CBSIZE) & ~CROUND;
	cc -= (int)q->c_cf;
	if (q->c_cc < cc)
		cc = q->c_cc;
	if (flag) {
		register char *p, *end;

		p = q->c_cf;
		end = p;
		end += cc;
		while (p < end) {
			if (*p & flag) {
				cc = (int)p;
				cc -= (int)q->c_cf;
				break;
			}
			p++;
		}
	}
out:
	splx(s);
	return (cc);
}

/*
 * Flush cc bytes from q.
 */
void
ndflush(q, cc)
	register struct clist *q;
	register int cc;
{
	register struct cblock *bp;
	char *end;
	int rem, s;

	s = spltty();
	if (q->c_cc <= 0)
		goto out;
	while (cc > 0 && q->c_cc) {
		bp = (struct cblock*) ((int) q->c_cf & ~CROUND);
		if ((int) bp == (((int) q->c_cl - 1) & ~CROUND)) {
			end = q->c_cl;
		} else {
			end = (char*) ((int) bp + sizeof(struct cblock));
		}
		rem = (u_char *)end - q->c_cf;
		if (cc >= rem) {
			cc -= rem;
			q->c_cc -= rem;
			q->c_cf = bp->c_next->c_info;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			if (cwaiting) {
				wakeup(&cwaiting);
				cwaiting = 0;
			}
		} else {
			q->c_cc -= cc;
			q->c_cf += cc;
			if (q->c_cc <= 0) {
				bp->c_next = cfreelist;
				cfreelist = bp;
				cfreecount += CBSIZE;
				if (cwaiting) {
					wakeup(&cwaiting);
					cwaiting = 0;
				}
			}
			break;
		}
	}
	if (q->c_cc <= 0) {
		q->c_cf = q->c_cl = NULL;
		q->c_cc = 0;
	}
out:
	splx(s);
}

int
putc(c, p)
	int c;
	register struct clist *p;
{
	register struct cblock *bp;
	register char *cp;
	register int s;

	s = spltty();
	if ((cp = p->c_cl) == NULL || p->c_cc < 0 ) {
		if ((bp = cfreelist) == NULL) {
			splx(s);
			return (-1);
		}
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
		p->c_cf = cp = bp->c_info;
	} else if (((int)cp & CROUND) == 0) {
		bp = (struct cblock *)cp - 1;
		if ((bp->c_next = cfreelist) == NULL) {
			splx(s);
			return (-1);
		}
		bp = bp->c_next;
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
		cp = bp->c_info;
	}
	*cp++ = c;
	p->c_cc++;
	p->c_cl = cp;
	splx(s);
	return (0);
}

/*
 * copy buffer to clist.
 * return number of bytes not transfered.
 */
int
b_to_q(cp, cc, q)
	register char *cp;
	struct clist *q;
	register int cc;
{
	register char *cq;
	register struct cblock *bp;
	register int s, nc;
	int acc;

	if (cc <= 0)
		return (0);
	acc = cc;
	s = spltty();
	if ((cq = q->c_cl) == NULL || q->c_cc < 0) {
		if ((bp = cfreelist) == NULL) 
			goto out;
		cfreelist = bp->c_next;
		cfreecount -= CBSIZE;
		bp->c_next = NULL;
		q->c_cf = cq = bp->c_info;
	}

	while (cc) {
		if (((int)cq & CROUND) == 0) {
			bp = (struct cblock *)cq - 1;
			if ((bp->c_next = cfreelist) == NULL) 
				goto out;
			bp = bp->c_next;
			cfreelist = bp->c_next;
			cfreecount -= CBSIZE;
			bp->c_next = NULL;
			cq = bp->c_info;
		}
		nc = MIN(cc, sizeof (struct cblock) - ((int)cq & CROUND));
		(void) bcopy(cp, cq, (unsigned)nc);
		cp += nc;
		cq += nc;
		cc -= nc;
	}
out:
	q->c_cl = cq;
	q->c_cc += acc - cc;
	splx(s);
	return (cc);
}

/*
 * Given a non-NULL pointter into the list (like c_cf which
 * always points to a real character if non-NULL) return the pointer
 * to the next character in the list or return NULL if no more chars.
 *
 * Callers must not allow getc's to happen between nextc's so that the
 * pointer becomes invalid.  Note that interrupts are NOT masked.
 */
char *
nextc(p, cp)
	register struct clist *p;
	register char *cp;
{
	register char *rcp;

	if (p->c_cc && ++cp != (char *)p->c_cl) {
		if (((int)cp & CROUND) == 0) {
			rcp = ((struct cblock *)cp)[-1].c_next->c_info;
		} else {
			rcp = cp;
		}
	} else
		rcp = (char *)NULL;
	return (rcp);
}

/*
 * Given a non-NULL pointer into the clist return the pointer
 * to the first character in the list or return NULL if no more chars.
 *
 * Callers must not allow getc's to happen between firstc's and getc's
 * so that the pointer becomes invalid.  Note that interrupts are NOT
 * masked.
 *
 * *c is set to the NEXT character
 */
char *
firstc(clp, c)
	register struct clist *clp;
	register int *c;
{
	u_char *cp;

	if (clp->c_cc == 0)
		return NULL;
	cp = clp->c_cf;
	*c = *cp & 0xff;
	if (clp->c_cq) {
#ifdef QBITS
		if (isset(clp->c_cq, cp - clp->c_cs))
			*c |= TTY_QUOTE;
#else
		if (*(cp - clp->c_cs + clp->c_cq))
			*c |= TTY_QUOTE;
#endif
	}
	return (clp->c_cf);
}

/*
 * Remove the last character in the list and return it.
 */
int
unputc(p)
	register struct clist *p;
{
	register struct cblock *bp;
	register int c, s;
	struct cblock *obp;

	s = spltty();
	if (p->c_cc <= 0)
		c = -1;
	else {
		c = *--p->c_cl;
		if (--p->c_cc <= 0) {
			bp = (struct cblock *)p->c_cl;
			bp = (struct cblock *)((int)bp & ~CROUND);
			p->c_cl = p->c_cf = NULL;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
		} else if (((int)p->c_cl & CROUND) == sizeof(bp->c_next)) {
			p->c_cl = (char *)((int)p->c_cl & ~CROUND);
			bp = (struct cblock *)p->c_cf;
			bp = (struct cblock *)((int)bp & ~CROUND);
			while (bp->c_next != (struct cblock *)p->c_cl)
				bp = bp->c_next;
			obp = bp;
			p->c_cl = (char *)(bp + 1);
			bp = bp->c_next;
			bp->c_next = cfreelist;
			cfreelist = bp;
			cfreecount += CBSIZE;
			cblock_free(bp);
			obp->c_next = NULL;
		}
	}
	splx(s);
	return (c);
}

/*
 * Put the chars in the from que
 * on the end of the to que.
 */
void
catq(from, to)
	register struct clist *from, *to;
{
	char bbuf[CBSIZE*4];
	register int c;
	int s;

	s = spltty();
	if (to->c_cc == 0) {
		*to = *from;
		from->c_cc = 0;
		from->c_cf = NULL;
		from->c_cl = NULL;
		splx(s);
		return;
	}
	splx(s);
	while (from->c_cc > 0) {
		c = q_to_b(from, bbuf, sizeof bbuf);
		(void) b_to_q(bbuf, c, to);
	}
}

#ifdef unneeded
/*
 * Integer (short) get/put using clists.
 * Note dependency on byte order.
 */
typedef	u_short word_t;

int
getw(p)
	register struct clist *p;
{
	register int s, c;
	register struct cblock *bp;

	if (p->c_cc <= 1)
		return(-1);
	if (p->c_cc & 01) {
		c = getc(p);
#if defined(vax)
		return (c | (getc(p)<<8));
#else
		return (getc(p) | (c<<8));
#endif
	}
	s = spltty();
#if defined(vax)
	c = *((word_t *)p->c_cf);
#else
	c = (((u_char *)p->c_cf)[1] << 8) | ((u_char *)p->c_cf)[0];
#endif
	p->c_cf += sizeof(word_t);
	p->c_cc -= sizeof(word_t);
	if (p->c_cc <= 0) {
		bp = (struct cblock*) (p->c_cf - 1);
		bp = (struct cblock*) ((int) bp & ~CROUND);
		p->c_cf = NULL;
		p->c_cl = NULL;
		bp->c_next = cfreelist;
		cfreelist = bp;
		cfreecount += CBSIZE;
		if (cwaiting) {
			wakeup(&cwaiting);
			cwaiting = 0;
		}
	} else if (((int) p->c_cf & CROUND) == 0) {
		bp = (struct cblock*) (p->c_cf);
		bp--;
		p->c_cf = bp->c_next->c_info;
		bp->c_next = cfreelist;
		cfreelist = bp;
		cfreecount += CBSIZE;
		if (cwaiting) {
			wakeup(&cwaiting);
			cwaiting = 0;
		}
	}
	splx(s);
	return (c);
}

int
putw(c, p)
	register struct clist *p;
	word_t c;
{
	register int s;
	register struct cblock *bp;
	register char *cp;

	s = spltty();
	if (cfreelist==NULL) {
		splx(s);
		return(-1);
	}
	if (p->c_cc & 01) {
#if defined(vax)
		(void) putc(c, p);
		(void) putc(c>>8, p);
#else
		(void) putc(c>>8, p);
		(void) putc(c, p);
#endif
	} else {
		if ((cp = p->c_cl) == NULL || p->c_cc < 0) {
			if ((bp = cfreelist) == NULL) {
				splx(s);
				return (-1);
			}
			cfreelist = bp->c_next;
			cfreecount -= CBSIZE;
			bp->c_next = NULL;
			p->c_cf = cp = bp->c_info;
		} else if (((int) cp & CROUND) == 0) {
			bp = (struct cblock*) cp - 1;
			if ((bp->c_next = cfreelist) == NULL) {
				splx(s);
				return (-1);
			}
			bp = bp->c_next;
			cfreelist = bp->c_next;
			cfreecount -= CBSIZE;
			bp->c_next = NULL;
			cp = bp->c_info;
		}
#if defined(vax)
		*(word_t *)cp = c;
#else
		((u_char *)cp)[0] = c>>8;
		((u_char *)cp)[1] = c;
#endif
		p->c_cl = cp + sizeof (word_t);
		p->c_cc += sizeof (word_t);
	}
	splx(s);
	return (0);
}
#endif /* unneeded */
