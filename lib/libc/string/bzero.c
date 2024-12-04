/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)bzero.c	1.1 (Berkeley) 1/19/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>

#if defined(_FORTIFY_SOURCE)
#undef bzero
#endif

int __bzero(char *, unsigned int);

/*
 * bzero -- vax movc5 instruction
 */
int
__bzero(b, length)
	char *b;
	unsigned int length;
{
	if (length) {
		do {
			*b++ = '\0';
		} while (--length);
	}
	return (length);
}

void
bzero(dst0, length)
	void *dst0;
	size_t length;
{
	char *p;

	p = (char*) dst0;
	(void) __bzero(p, length);
}
