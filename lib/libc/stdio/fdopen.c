/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)fdopen.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

/*
 * Unix routine to do an "fopen" on file descriptor
 * The mode has to be repeated because you can't query its
 * status
 */

#include <sys/types.h>
#include <sys/stat.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <limits.h>

#include "local.h"

FILE *
fdopen(fd, mode)
	int fd;
const  char *mode;
{
	extern FILE *_findiop();
	static int nofile = -1;
	register FILE *iop;

	if (nofile < 0)
		nofile = getdtablesize();

	if (fd < 0 || fd >= nofile)
		return (NULL);

	iop = _findiop();
	if (iop == NULL)
		return (NULL);

	iop->_file = fd;
	iop->_cookie = fp;
	iop->_read = __sread;
	iop->_write = __swrite;
	iop->_seek = __sseek;
	iop->_close = __sclose;

	switch (*mode) {
	case 'r':
		iop->_flags = _IOREAD;
		break;
	case 'a':
		lseek(fd, (off_t)0, L_XTND);
		/* fall into ... */
	case 'w':
		iop->_flags = _IOWRT;
		break;
	default:
		return (NULL);
	}

	if (mode[1] == '+')
		iop->_flags = _IORW;

	return (iop);
}
