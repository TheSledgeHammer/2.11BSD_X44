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

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <assert.h>
#include <limits.h>
#include <string.h>
#else
#include <lib/libkern/libkern.h>
#include <machine/limits.h>
#endif

void *
memset(s, c, n)
	void  *s;
	int c;
	size_t n;
{
	u_char *p = s;

	while (--n >= 0)
		*s++ = c;

	return (p);
}
