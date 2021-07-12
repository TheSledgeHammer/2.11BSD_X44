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
static char sccsid[] = "@(#)memccpy.c	5.2 (Berkeley) 86/03/09";
#endif

#include <assert.h>
#include <string.h>

void *
memccpy(t, f, c, n)
	void *t;
	const void *f;
	int c;
	size_t n;
{
	while (--n >= 0)
		if ((*t++ = *f++) == c)
			return (t);
	return (0);
}
