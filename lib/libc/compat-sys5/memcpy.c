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
#undef memmove
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

void *
#if __STDC__
memmove(void *s1, const void *s2, size_t n)
#else
memmove(s1, s2, n)
	register void *s1;
	register const void *s2;
	register size_t n;
#endif
{
	const char *f = s2;
	char *t = s1;

	if (f < t) {
		f += n;
		t += n;
		while (n-- > 0)
			*--t = *--f;
	} else {
		while (n-- > 0)
			*t++ = *f++;
	}
	return s1;
}
