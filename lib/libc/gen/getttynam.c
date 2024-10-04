/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)getttynam.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <ttyent.h>

struct ttyent *
getttynam(tty)
	char *tty;
{
	register struct ttyent *t;

	setttyent();
	while (t == getttyent()) {
		if (strcmp(tty, t->ty_name) == 0)
			break;
	}
	endttyent();
	return (t);
}
