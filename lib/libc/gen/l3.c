/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)l3.c	2.3 (Berkeley) 1/25/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>

/*
 * Convert longs to 3-byte disk addresses
 */
void
ltol3(cp, lp, n)
	char *cp;
	long *lp;
	int n;
{
	register int i;
	register char *a, *b;

	a = cp;
	b = (char *)lp;
	for (i=0; i<n; i++) {
#ifdef interdata
		b++;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
#else
		*a++ = *b++;
		b++;
		*a++ = *b++;
		*a++ = *b++;
#endif
	}
}

/*
 * Convert 3-byte disk addresses to longs
 */
void
l3tol(lp, cp, n)
	long *lp;
	char *cp;
	int n;
{
	register int i;
	register char *a, *b;

	a = (char *)lp;
	b = cp;
	for(i=0; i<n; i++) {
#ifdef interdata
		*a++ = 0;
		*a++ = *b++;
		*a++ = *b++;
		*a++ = *b++;
#else
		*a++ = *b++;
		*a++ = 0;
		*a++ = *b++;
		*a++ = *b++;
#endif
	}
}
