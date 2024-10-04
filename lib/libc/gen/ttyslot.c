/*
 * Copyright (c) 1984 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)ttyslot.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Return the number of the slot in the utmp file
 * corresponding to the current user: try for file 0, 1, 2.
 * Definition is the line number in the /etc/ttys file.
 */
#include <ttyent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

ttyslot()
{
	register struct ttyent *ty;
	register char *tp, *p;
	register s;

	if ((tp = ttyname(0)) == NULL &&
	    (tp = ttyname(1)) == NULL &&
	    (tp = ttyname(2)) == NULL)
		return(0);
	if ((p = rindex(tp, '/')) == NULL)
		p = tp;
	else
		p++;
	setttyent();
	s = 0;
	while ((ty = getttyent()) != NULL) {
		s++;
		if (strcmp(ty->ty_name, p) == 0) {
			endttyent();
			return (s);
		}
	}
	endttyent();
	return (0);
}
