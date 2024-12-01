/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)ffs.c	1.1 (Berkeley) 1/19/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <string.h>

/* #undef ffs() - might be defined as macro to __builtin_ffs() */
#undef ffs

/*
 * ffs -- vax ffs instruction
 */
int
ffs(mask)
	register long mask;
{
	register int cnt;

	if (mask == 0)
		return (0);
	for (cnt = 1; !(mask & 1); cnt++)
		mask >>= 1;
	return (cnt);
}
