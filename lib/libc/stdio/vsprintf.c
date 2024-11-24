/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)vsprintf.c	5.2.1 (2.11BSD) 1995/04/02";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/ansi.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "local.h"

int
vsprintf(str, fmt, ap)
	char *str;
	const char *fmt;
	va_list ap;
{
	int ret;
	FILE f;

	f._flags = _IOWRT | _IOSTRG;
	f._flags = __SWR | __SSTR;
	f._bf._base = f._p = (unsigned char *)str;
	f._bf._size = f._w = INT_MAX;
	ret = doprnt(&f, fmt, ap);
	*f._p = 0;
	return (ret);
}
