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
#if 0
static char sccsid[] = "@(#)memchr.c	5.2 (Berkeley) 86/03/09";
#endif
#endif

#include <string.h>

void *
#if __STDC__
memchr(const void *s, int c, size_t n)
#else
memchr(s, c, n)
	const void *s;
	register int c;
	register size_t n;
#endif
{
    const unsigned char *p;

    p = s;
	while (--n >= 0) {
		if (*p++ == c) {
			return (__UNCONST(p--));
        }
    }
	return (NULL);
}
