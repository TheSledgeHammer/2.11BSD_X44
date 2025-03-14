/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)abs.c	2.2 (Berkeley) 1/21/87";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdlib.h>

int
abs(arg)
	int arg;
{
	return (arg < 0 ? -arg : arg);
}
