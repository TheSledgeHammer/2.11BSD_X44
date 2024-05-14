/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)mult.c	5.1 (Berkeley) 4/30/85";
#endif not lint

#include <mp.h>

union mult {
	long 		xx;
	struct half yy;
};

void
mult(a, b, c)
	struct mint *a, *b, *c;
{
	struct mint x, y, z;
	int sign;
	sign = 1;
	x.val = a->val;
	y.val = b->val;
	z.len = 0;
	if (a->len < 0) {
		x.len = -a->len;
		sign = -sign;
	} else
		x.len = a->len;
	if (b->len < 0) {
		y.len = -b->len;
		sign = -sign;
	} else
		y.len = b->len;
	if (x.len < y.len)
		m_mult(&y, &x, &z);
	else
		m_mult(&x, &y, &z);
	xfree(c);
	if (sign < 0)
		c->len = -z.len;
	else
		c->len = z.len;
	if (c->len == 0)
		shfree(z.val);
	else
		c->val = z.val;
	return;
}

#define S2(x, a, j)			((x) = (a)->val[(j)])
#define S3(x, b, i, j)		((x) = (x) * (b)->val[(i) - (j)])
#define S4(carry, sum, x)	(tradd(carry, sum, x))
#define S5(c, i, sum) 		((c)->val[(i)] = (sum).yy.low & 077777)
#define S6(sum) 			((sum).xx = (sum).xx >> 15)
#define S7(sum, carry) 		((sum).yy.high = (carry))

void
m_mult(a, b, c)
	struct mint *a, *b, *c;
{
	long x;
	union mult sum;
	int carry;
	int i, j;

	c->val = xalloc(a->len + b->len, "m_mult");
	sum.xx = 0;
	for (i = 0; i < b->len; i++) {
		carry = 0;
		for (j = 0; j < i + 1; j++) {
			S2(x, a, j);
			S3(x, b, i, j);
			S4(&carry, &sum, x);
		}
		S5(c, i, sum);
		S6(sum);
		S7(sum, carry);
	}
	for (; i < a->len; i++) {
		carry = 0;
		for (j = i - b->len + 1; j < i + 1; j++) {
			S2(x, a, j);
			S3(x, b, i, j);
			S4(&carry, &sum, x);
		}
		S5(c, i, sum);
		S6(sum);
		S7(sum, carry);
	}
	for (; i < a->len + b->len; i++) {
		carry = 0;
		for (j = i - b->len + 1; j < a->len; j++) {
			S2(x, a, j);
			S3(x, b, i, j);
			S4(&carry, &sum, x);
		}
		S5(c, i, sum);
		S6(sum);
		S7(sum, carry);
	}
	if (c->val[i - 1] != 0)
		c->len = a->len + b->len;
	else
		c->len = a->len + b->len - 1;
	return;
}

void
tradd(a, b, c)
	long c;
	int *a;
	union mult *b;
{
	b->xx = b->xx + c;
	if (b->yy.high & 0100000) {
		b->yy.high = b->yy.high & 077777;
		*a += 1;
	}
	return;
}
