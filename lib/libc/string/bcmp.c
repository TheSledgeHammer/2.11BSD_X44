/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)bcmp.c	1.1 (Berkeley) 1/19/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>

int __bcmp(const char *, const char *, unsigned int);

/*
 * bcmp -- vax cmpc3 instruction
 */
int
__bcmp(b1, b2, length)
	register const char *b1, *b2;
	register unsigned int length;
{
	if (length) {
		do {
			if (*b1++ != *b2++) {
				break;
			}
		} while (--length);
	}
	return (length);
}

int
bcmp(b1, b2, length)
	const void *b1;
	const void *b2;
	size_t length;
{
	const char *p1, *p2;

	p1 = (const char*) b1;
	p2 = (const char*) b2;
	return (__bcmp(p1, p2, length));
}
