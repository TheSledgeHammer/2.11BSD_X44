/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)nice.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(nice,_nice)
#endif

/*
 * Backwards compatible nice.
 */
int
#if __STDC__
nice(int incr)
#else
nice(incr)
	int incr;
#endif
{
	int prio;
	//extern int errno;

	errno = 0;
	prio = getpriority(PRIO_PROCESS, 0);
	if (prio == -1 && errno)
		return (-1);
	return (setpriority(PRIO_PROCESS, 0, prio + incr));
}
