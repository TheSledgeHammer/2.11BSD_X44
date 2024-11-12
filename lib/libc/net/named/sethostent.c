/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)sethostent.c	6.3 (Berkeley) 4/10/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/nameser.h>

#include <netdb.h>
#include <resolv.h>

void endhostent(void);
void sethostfile(const char *);

void
sethostent(stayopen)
    int stayopen;
{
	if (stayopen)
		_res.options |= RES_STAYOPEN | RES_USEVC;
}

void
endhostent(void)
{
	_res.options &= ~(RES_STAYOPEN | RES_USEVC);
//	res_close();
}

void
sethostfile(name)
	const char *name;
{
#ifdef lint
	name = name;
#endif
}
