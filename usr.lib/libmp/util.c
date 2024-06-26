/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)util.c	5.1 (Berkeley) 4/30/85";
#endif not lint

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <mp.h>

void
move(a, b)
	MINT *a, *b;
{
	int i, j;
	xfree(b);
	b->len = a->len;
	if ((i = a->len) < 0)
		i = -i;
	if (i == 0)
		return;
	b->val = xalloc(i, "move");
	for (j = 0; j < i; j++) {
		b->val[j] = a->val[j];
	}
	return;
}

dummy()
{

}

short *
xalloc(nint, s)
	int nint;
	char *s;
{
	short *i;
	i = (short*) malloc(2 * (unsigned) nint + 4);
#ifdef DBG
	if(dbg) fprintf(stderr, "%s: %o\n",s,i);
#endif
	if (i != NULL)
		return (i);
	fatal("mp: no free space");
	return (0);
}

void
fatal(s)
	char *s;
{
	fprintf(stderr, "%s\n", s);
	(void)fflush(stdout);
	sleep(2);
	abort();
}

void
xfree(c)
	MINT *c;
{
#ifdef DBG
	if(dbg) fprintf(stderr, "xfree ");
#endif
	if (c->len == 0)
		return;
	shfree(c->val);
	c->len = 0;
	return;
}

void
mcan(a)
	MINT *a;
{
	int i, j;
	if ((i = a->len) == 0)
		return;
	else if (i < 0)
		i = -i;
	for (j = i; j > 0 && a->val[j - 1] == 0; j--)
		;
	if (j == i)
		return;
	if (j == 0) {
		xfree(a);
		return;
	}
	if (a->len > 0)
		a->len = j;
	else
		a->len = -j;
}

MINT *
itom(n)
	int n;
{
	MINT *a;
	a = (MINT*) xalloc(2, "itom");
	if (n > 0) {
		a->len = 1;
		a->val = xalloc(1, "itom1");
		*a->val = n;
		return (a);
	} else if (n < 0) {
		a->len = -1;
		a->val = xalloc(1, "itom2");
		*a->val = -n;
		return (a);
	} else {
		a->len = 0;
		return (a);
	}
}

int
mcmp(a, b)
	MINT *a, *b;
{
	MINT c;
	int res;
	if (a->len != b->len)
		return (a->len - b->len);
	c.len = 0;
	msub(a, b, &c);
	res = c.len;
	xfree(&c);
	return (res);
}
