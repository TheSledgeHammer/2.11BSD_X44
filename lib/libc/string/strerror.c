/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
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
static char sccsid[] = "@(#)strerror.c	8.1.1 (2.11BSD) 1996/3/15";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <errlst.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

#ifdef __weak_alias
__weak_alias(strerror,_strerror)
__weak_alias(strerror_r,_strerror_r)
#endif

#define	EBUFSIZE	40
#define	UPREFIX		"Unknown error: "

static char *error2string(int, const char *, char *);

static char *
error2string(num, uprefix, ebuf)
	int num;
	const char *uprefix;
	char *ebuf;
{
	extern int sys_nerr;
	register unsigned int errnum;
	register char *p, *t;
	char tmp[EBUFSIZE];

	errnum = num; /* convert to unsigned */
	if (errnum < sys_nerr) {
		return (syserrlst(errnum));
	}

	/* Do this by hand, so we don't include stdio(3). */
	t = tmp;
	do {
		//*t++ = '0' + (errnum % 10);
		*t++ = "0123456789"[errnum % 10];
	} while (errnum /= 10);
	for (p = ebuf + sizeof(uprefix) - 1;;) {
		*p++ = *--t;
		if (t <= tmp) {
			break;
		}
	}
	return (ebuf);
}

int
strerror_r(num, buf, buflen)
	int num;
	char *buf;
	size_t buflen;
{
	char *error;
	size_t slen;
	int rval;

	rval = 0;
	if (num >= 0) {
		error = error2string(num, UPREFIX, buf);
		slen = strlcpy(buf, error, buflen);
		if (slen > buflen) {
			rval = ERANGE;
		}
	} else {
		//slen = snprintf(buf, buflen, UPREFIX, num);
		rval = EINVAL;
	}
	return (rval);
}

char *
strerror(num)
	int num;
{
	static char ebuf[EBUFSIZE] = UPREFIX; /* 64-bit number + slop */
	if (strerror_r(num, ebuf, sizeof(ebuf)) != 0) {
		errno = EINVAL;
	}
	return (ebuf);
}
