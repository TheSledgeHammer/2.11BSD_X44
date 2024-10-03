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
static char sccsid[] = "@(#)memcmp.c	5.2 (Berkeley) 86/03/09";
#endif
#endif

#include <string.h>

int
#if __STDC__
memcmp(const void *s1, const void *s2, size_t n)
#else
memcmp(s1, s2, n)
	const void *s1, *s2;
	size_t n;
#endif
{
    const unsigned char *p1, *p2;

    p1 = s1;
    p2 = s2;
	while (--n >= 0) {
		if (*p1++ != *p2++) {
			return (*--p1 - *--p2);
        }
    }
	return (0);
}
