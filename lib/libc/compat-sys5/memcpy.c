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
static char sccsid[] = "@(#)memcpy.c	5.2 (Berkeley) 86/03/09";
#endif
#endif

#include <assert.h>
#include <string.h>

#if defined(_FORTIFY_SOURCE)
#undef memcpy
#endif

void *
#if __STDC__
memcpy(void *t, const void *f, size_t n)
#else
memcpy(t, f, n)
	register void *t;
    register const void *f;
	register size_t n;
#endif
{
    const char *ff;
    char *tt;

    tt = t;
    ff = f;
	while (--n >= 0) {
		*tt++ = *ff++;
    }
	return (t);
}
