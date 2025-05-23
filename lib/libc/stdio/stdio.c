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
static char sccsid[] = "@(#)stdio.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "reentrant.h"
#include "local.h"

/*
 * Small standard I/O/seek/close functions.
 * These maintain the `known seek offset' for seek optimisation.
 */
int
__sread(cookie, buf, n)
	void *cookie;
	char *buf;
	int n;
{
	register FILE *fp = cookie;
	register int ret;
	
	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(buf != NULL);

	ret = read(fp->_file, buf, n);
	/* if the read succeeded, update the current offset */
	if (ret >= 0)
		fp->_offset += ret;
	else
		fp->_flags &= ~__SOFF;	/* paranoia */
	return (ret);
}

int
__swrite(cookie, buf, n)
	void *cookie;
	char const *buf;
	int n;
{
	register FILE *fp = cookie;

	_DIAGASSERT(cookie != NULL);
	_DIAGASSERT(buf != NULL);

	if (fp->_flags & __SAPP)
		(void) lseek(fp->_file, (off_t)0, SEEK_END);
	fp->_flags &= ~__SOFF;	/* in case FAPPEND mode is set */
	return (write(fp->_file, buf, n));
}

fpos_t
__sseek(cookie, offset, whence)
	void *cookie;
	fpos_t offset;
	int whence;
{
	register FILE *fp = cookie;
	register off_t ret;
	
	_DIAGASSERT(fp != NULL);

	ret = lseek(fp->_file, (off_t)offset, whence);
	if (ret == -1L)
		fp->_flags &= ~__SOFF;
	else {
		fp->_flags |= __SOFF;
		fp->_offset = ret;
	}
	return (ret);
}

int
__sclose(cookie)
	void *cookie;
{
	_DIAGASSERT(cookie != NULL);

	return (close(((FILE *)cookie)->_file));
}
