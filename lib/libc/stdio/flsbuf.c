/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include	<sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)flsbuf.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include	<sys/types.h>
#include	<sys/stat.h>

#include	<stdio.h>
#include 	<stddef.h>

char	*malloc();

int
_flsbuf(c, iop)
	unsigned char c;
	register FILE *iop;
{
	register char *base;
	register n, rn;
	char c1;
	int size;
	struct stat stbuf;

	if (iop->_flags & _IORW) {
		iop->_flags |= _IOWRT;
		iop->_flags &= ~(_IOEOF | _IOREAD);
	}

	if ((iop->_flags & _IOWRT) == 0)
		return (EOF);
tryagain:
	if (iop->_flags & _IOLBF) {
		base = iop->_base;
		*iop->_p++ = c;
		if (iop->_p >= base + iop->_bufsiz || c == '\n') {
			n = write(fileno(iop), base, rn = iop->_p - base);
			iop->_p = base;
			iop->_cnt = 0;
		} else
			rn = n = 0;
	} else if (iop->_flags & _IONBF) {
		c1 = c;
		rn = 1;
		n = write(fileno(iop), &c1, rn);
		iop->_cnt = 0;
	} else {
		if ((base = iop->_base) == NULL) {
			if (fstat(fileno(iop), &stbuf) < 0 || stbuf.st_blksize <= NULL)
				size = BUFSIZ;
			else
				size = stbuf.st_blksize;
			if ((iop->_base = base = malloc(size)) == NULL) {
				iop->_flags |= _IONBF;
				goto tryagain;
			}
			iop->_flags |= _IOMYBUF;
			iop->_bufsiz = size;
			if (iop == stdout && isatty(fileno(stdout))) {
				iop->_flags |= _IOLBF;
				iop->_p = base;
				goto tryagain;
			}
			rn = n = 0;
		} else if ((rn = n = iop->_p - base) > 0) {
			iop->_p = base;
			n = write(fileno(iop), base, n);
		}
		iop->_cnt = iop->_bufsiz - 1;
		*base++ = c;
		iop->_p = base;
	}
	if (rn != n) {
		iop->_flags |= _IOERR;
		return (EOF);
	}
	return (c);
}

int
fflush(iop)
	register FILE *iop;
{
	register char *base;
	register n;

	if ((iop->_flags&(_IONBF|_IOWRT))==_IOWRT
			&& (base = iop->_base) != NULL && (n = iop->_p - base) > 0) {
		iop->_p = base;
		iop->_cnt = (iop->_flags & (_IOLBF | _IONBF)) ? 0 : iop->_bufsiz;
		if (write(fileno(iop), base, n) != n) {
			iop->_flags |= _IOERR;
			return (EOF);
		}
	}
	return (0);
}

int
fclose(iop)
	register FILE *iop;
{
	register int r;

	r = EOF;
	if ((iop->_flag & (_IOREAD | _IOWRT | _IORW))
			&& (iop->_flag & _IOSTRG) == 0) {
		r = fflush(iop);
		if (close(fileno(iop)) < 0)
			r = EOF;
		if (iop->_flag & _IOMYBUF)
			free(iop->_base);
	}
	iop->_cnt = 0;
	iop->_base = (char*) NULL;
	iop->_ptr = (char*) NULL;
	iop->_bufsiz = 0;
	iop->_flag = 0;
	iop->_file = 0;
	return (r);
}
