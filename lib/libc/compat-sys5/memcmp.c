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
static char sccsid[] = "@(#)memcmp.c	5.2 (Berkeley) 86/03/09";
#endif

int
memcmp(s1, s2, n)
	const void *s1, *s2;
	size_t n;
{
	while (--n >= 0)
		if (*s1++ != *s2++)
			return (*--s1 - *--s2);
	return (0);
}
