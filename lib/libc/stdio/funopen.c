/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)funopen.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>

#include "reentrant.h"
#include "local.h"

typedef int (*readfn_t)(void *, char *, int);
typedef int (*writefn_t)(void *, const char *, int);
typedef fpos_t (*seekfn_t)(void *, fpos_t, int);
typedef int (*closefn_t)(void *);
typedef int	(*flushfn_t)(void *);

FILE *
funopen(cookie, readfn, writefn, seekfn, closefn)
	const void *cookie;
	readfn_t readfn;
	writefn_t writefn;
	seekfn_t seekfn;
	closefn_t closefn;
{
	return (funopen2(cookie, readfn, writefn, seekfn, NULL, closefn));
}

FILE *
funopen2(cookie, readfn, writefn, seekfn, flushfn, closefn)
	const void *cookie;
	readfn_t readfn;
	writefn_t writefn;
	seekfn_t seekfn;
	flushfn_t flushfn;
	closefn_t closefn;
{
	register FILE *fp;
	int flags;

	if (readfn == NULL) {
		if (writefn == NULL) {		/* illegal */
			errno = EINVAL;
			return (NULL);
		} else
			flags = __SWR;		/* write only */
	} else {
		if (writefn == NULL)
			flags = __SRD;		/* read only */
		else
			flags = __SRW;		/* read-write */
	}
	if ((fp = __sfp()) == NULL)
		return (NULL);
	fp->_flags = flags;
	fp->_file = -1;
	fp->_cookie = __UNCONST(cookie);
	fp->_read = readfn;
	fp->_write = writefn;
	fp->_seek = seekfn;
	fp->_close = closefn;
	fp->_flush = flushfn;

	return (fp);
}
