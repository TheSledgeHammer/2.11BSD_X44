/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)utime.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/time.h>

#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <utime.h>

/*
 * Backwards compatible utime.
 */
int
#if __STDC__
utime(const char *name, const struct utimbuf *otv)
#else
utime(name, otv)
	const char *name;
	const struct utimbuf *otv;
#endif
{
	struct timeval tv[2], *tvp;

	if (otv == (struct utimbuf *) NULL) {
		tvp = NULL;
	} else {
		tv[0].tv_sec = otv->actime;
		tv[1].tv_sec = otv->modtime;
		tv[0].tv_usec = tv[1].tv_usec = 0;
		tvp = tv;
	}
	return (utimes(name, tvp));
}
