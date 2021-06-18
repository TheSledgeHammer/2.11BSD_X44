/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fprintf.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stddef.h>

int
fprintf(iop, fmt, args)
	register FILE *iop;
	char 	*fmt;
	va_list args;

{
	char localbuf[BUFSIZ];

	if (iop->_flags & _IONBF) {
		iop->_flags &= ~_IONBF;
		iop->_ptr = iop->_base = localbuf;
		iop->_bufsiz = BUFSIZ;
		doprnt(fmt, &args, iop);
		fflush(iop);
		iop->_flag |= _IONBF;
		iop->_base = NULL;
		iop->_bufsiz = NULL;
		iop->_cnt = 0;
	} else {
		doprnt(fmt, &args, iop);
	}
	return(ferror(iop)? EOF: 0);
}
