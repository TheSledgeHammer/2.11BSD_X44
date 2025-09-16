/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)ftime.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>

/*
 * Backwards compatible ftime.
 */

/* from old timeb.h */
/*
struct timeb {
	time_t	time;
	u_short	millitm;
	short	timezone;
	short	dstflag;
};
*/

int
#if __STDC__
ftime(register struct timeb *tp)
#else
ftime(tp)
	register struct timeb *tp;
#endif
{
	struct timeval t;
	struct timezone tz;

	if (gettimeofday(&t, &tz) < 0) {
		return (-1);
	}
	tp->time = t.tv_sec;
	tp->millitm = t.tv_usec / 1000;
	tp->timezone = tz.tz_minuteswest;
	tp->dstflag = tz.tz_dsttime;
	return (0);
}
