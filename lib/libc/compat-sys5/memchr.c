/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Sys5 compat routine
 */
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)memchr.c	5.2 (Berkeley) 86/03/09";
#endif

void *
memchr(s, c, n)
	const void *s;
	register unsigned char c;
	register size_t n;
{
	while (--n >= 0)
		if (*s++ == c)
			return (--s);
	return (0);
}
