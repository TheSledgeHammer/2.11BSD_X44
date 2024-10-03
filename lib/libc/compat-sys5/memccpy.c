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
static char sccsid[] = "@(#)memccpy.c	5.2 (Berkeley) 86/03/09";
#endif
#endif

#include <string.h>

void *
#if __STDC__
memccpy(void *t, const void *f, int c, size_t n)
#else
memccpy(t, f, c, n)
	void *t;
	const void *f;
	int c;
	size_t n;
#endif
{
    unsigned char *tp;
    const unsigned char *fp;

    tp = t;
    fp = f;
	while (--n >= 0) {
		if ((*tp++ = *fp++) == c) {
			return (t);
        }
    }
	return (0);
}
