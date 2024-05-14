/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)mout.c	5.2 (Berkeley) 3/13/86";
#endif not lint

#include <stdio.h>
#include <stdlib.h>
#include <mp.h>

int
m_in(a, b, f)
	MINT *a;
	FILE *f;
	short b;
{
	MINT x, y, ten;
	int sign, c;
	short qten, qy;

	xfree(a);
	sign = 1;
	ten.len = 1;
	ten.val = &qten;
	qten = b;
	x.len = 0;
	y.len = 1;
	y.val = &qy;
	while ((c = getc(f)) != EOF)
		switch (c) {
		case '\\':
			(void) getc(f);
			continue;
		case '\t':
		case '\n':
			a->len *= sign;
			xfree(&x);
			return (0);
		case ' ':
			continue;
		case '-':
			sign = -sign;
			continue;
		default:
			if (c >= '0' && c <= '9') {
				qy = c - '0';
				mult(&x, &ten, a);
				madd(a, &y, a);
				move(a, &x);
				continue;
			} else {
				(void) ungetc(c, stdin);
				a->len *= sign;
				return (0);
			}
		}
	return (EOF);
}

void
m_out(a, b, f)
	MINT *a;
	FILE *f;
	int b;
{
	int sign, xlen, i;
	short r;
	MINT x;
	char *obuf;
	register char *bp;

	sign = 1;
	xlen = a->len;
	if (xlen < 0) {
		xlen = -xlen;
		sign = -1;
	}
	if (xlen == 0) {
		fprintf(f, "0\n");
		return;
	}
	x.len = xlen;
	x.val = xalloc(xlen, "m_out");
	for (i = 0; i < xlen; i++)
		x.val[i] = a->val[i];
	obuf = malloc(7 * (unsigned) xlen);
	bp = obuf + 7 * xlen - 1;
	*bp-- = 0;
	while (x.len > 0) {
		for (i = 0; i < 10 && x.len > 0; i++) {
			sdiv(&x, (short) b, &x, &r);
			*bp-- = r + '0';
		}
		if (x.len > 0)
			*bp-- = ' ';
	}
	if (sign == -1)
		*bp-- = '-';
	fprintf(f, "%s\n", bp + 1);
	free(obuf);
	FREE(x)
	return;
}

void
sdiv(a, n, q, r)
	MINT *a, *q;
	short n;
	short *r;
{
	MINT x, y;
	int sign;

	sign = 1;
	x.len = a->len;
	x.val = a->val;
	if (n < 0) {
		sign = -sign;
		n = -n;
	}
	if (x.len < 0) {
		sign = -sign;
		x.len = -x.len;
	}
	s_div(&x, n, &y, r);
	xfree(q);
	q->val = y.val;
	q->len = sign * y.len;
	*r = *r * sign;
	return;
}

void
s_div(a, n, q, r)
	MINT *a, *q;
	short n;
	short *r;
{
	int qlen, i;
	long int x;
	short *qval;

	x = 0;
	qlen = a->len;
	qval = xalloc(qlen, "s_div");
	for (i = qlen - 1; i >= 0; i--) {
		x = x * 0100000L + a->val[i];
		qval[i] = x / n;
		x = x % n;
	}
	*r = x;
	if (qlen && qval[qlen - 1] == 0)
		qlen--;
	q->len = qlen;
	q->val = qval;
	if (qlen == 0)
		shfree(qval);
	return;
}

int
min(a)
	MINT *a;
{
	return (m_in(a, 10, stdin));
}

int
omin(a)
	MINT *a;
{
	return (m_in(a, 8, stdin));
}

void
mout(a)
	MINT *a;
{
	m_out(a, 10, stdout);
}

void
omout(a)
	MINT *a;
{
	m_out(a, 8, stdout);
}

void
fmout(a, f)
	MINT *a;
	FILE *f;
{
	m_out(a, 10, f);
}

int
fmin(a, f)
	MINT *a;
	FILE *f;
{
	return (m_in(a, 10, f));
}
