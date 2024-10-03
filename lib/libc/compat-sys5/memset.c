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
static char sccsid[] = "@(#)memset.c	5.2 (Berkeley) 86/03/09";
#endif
#endif

#include <sys/types.h>

#include <assert.h>
#include <limits.h>
#include <string.h>

#undef memset

void *
#if __STDC__
memset(void *s, int c, size_t n)
#else
memset(s, c, n)
	void  *s;
	int c;
	size_t n;
#endif
{
	unsigned char *p;

    p = s;
	while (--n >= 0) {
		*p++ = c;
    }
	return (s);
}
