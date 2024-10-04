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

/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */
#include <sys/signal.h>
#include <string.h>
#include <unistd.h>

extern	char *sys_siglist[];

void
psignal(sig, s)
	unsigned sig;
	char *s;
{
	register char *c;
	register n;

	c = "Unknown signal";
	if (sig < NSIG)
		c = sys_siglist[sig];
	else
		c = "Unknown signal";
	n = strlen(s);
	if (n) {
		write(2, s, n);
		write(2, ": ", 2);
	}
	write(2, c, strlen(c));
	write(2, "\n", 1);
}
