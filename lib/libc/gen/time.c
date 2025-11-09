/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)time.c	5.3 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

/*
 * Backwards compatible time call.
 */
#include <sys/types.h>
#include <sys/time.h>

#include <time.h>

#ifdef __weak_alias
__weak_alias(time, _time)
#endif

time_t
time(t)
	time_t *t;
{
	struct timeval tt;

	if (gettimeofday(&tt, (struct timezone *)0) < 0)
		return ((time_t)-1);
	if (t)
		*t = (time_t)tt.tv_sec;
	return ((time_t)tt.tv_sec);
}
