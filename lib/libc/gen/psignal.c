/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)psignal.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>
#include <sys/uio.h>
/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */
#include <signal.h>
#include <string.h>
#include <unistd.h>

#ifdef __weak_alias
__weak_alias(psignal,_psignal)
#endif

void
psignal(sig, s)
	unsigned int sig;
	const char *s;
{
	register const char *c;
	register int n;

	c = "Unknown signal";
	if (sig < NSIG)
		c = sys_siglist[sig];
	else
		c = "Unknown signal";
	n = strlen(s);
	if (n) {
		write(STDERR_FILENO, s, n);
		write(STDERR_FILENO, ": ", 2);
	}
	write(STDERR_FILENO, c, strlen(c));
	write(STDERR_FILENO, "\n", 1);
}
