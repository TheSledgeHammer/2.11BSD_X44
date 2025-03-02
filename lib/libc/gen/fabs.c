/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)fabs.c	2.2 (Berkeley) 1/25/87";
#endif
#endif /* LIBC_SCCS and not lint */

/* Already defined in libc/arch/"arch"/gen */
double fabs(double);

double
fabs(arg)
	double arg;
{
	return (arg < 0 ? -arg : arg);
}
