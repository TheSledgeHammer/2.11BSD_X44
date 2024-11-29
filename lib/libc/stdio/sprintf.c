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

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#include "reentrant.h"
#include "local.h"

int
sprintf(char *str, char const *fmt, ...)
{
	int ret;
	va_list ap;
	FILE _strbuf;
	struct __sfileext fext;

	_DIAGASSERT(str != NULL);
	_DIAGASSERT(fmt != NULL);

	_FILEEXT_SETUP(&_strbuf, &fext);

	_strbuf._flags = _IOWRT | _IOSTRG;
	_strbuf._flags = __SWR | __SSTR;
	_strbuf._bf._base = _strbuf._p = (unsigned char *)str;
	_strbuf._bf._size = _strbuf._w = INT_MAX;
	va_start(ap, fmt);
	ret = doprnt(&_strbuf, fmt, ap);
	va_end(ap);
	*_strbuf._p = 0;
	return (ret);
}
