/*	$NetBSD: gettemp.c,v 1.13 2003/12/05 00:57:36 uebayasi Exp $	*/

/*
 * Copyright (c) 1987, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include "gettemp.h"

#if !HAVE_NBTOOL_CONFIG_H || !HAVE_MKSTEMP || !HAVE_MKDTEMP

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)mktemp.c	8.1 (Berkeley) 6/4/93";
#else
__RCSID("$NetBSD: gettemp.c,v 1.13 2003/12/05 00:57:36 uebayasi Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <fcntl.h>
#include <string.h>

int
GETTEMP(path, doopen, domkdir)
	char *path;
	int *doopen;
	int domkdir;
{
	char *start, *trv;
	struct stat sbuf;
	u_int pid;

	/* To guarantee multiple calls generate unique names even if
	   the file is not created. 676 different possibilities with 7
	   or more X's, 26 with 6 or less. */
	static char xtra[2] = "aa";
	int xcnt = 0;

	_DIAGASSERT(path != NULL);
	/* doopen may be NULL */

	pid = getpid();

	/* Move to end of path and count trailing X's. */
	for (trv = path; *trv; ++trv)
		if (*trv == 'X')
			xcnt++;
		else
			xcnt = 0;	

	/* Use at least one from xtra.  Use 2 if more than 6 X's. */
	if (*(trv - 1) == 'X')
		*--trv = xtra[0];
	if (xcnt > 6 && *(trv - 1) == 'X')
		*--trv = xtra[1];

	/* Set remaining X's to pid digits with 0's to the left. */
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/* update xtra for next call. */
	if (xtra[0] != 'z')
		xtra[0]++;
	else {
		xtra[0] = 'a';
		if (xtra[1] != 'z')
			xtra[1]++;
		else
			xtra[1] = 'a';
	}

	/*
	 * check the target directory; if you have six X's and it
	 * doesn't exist this runs for a *very* long time.
	 */
	for (start = trv + 1;; --trv) {
		if (trv <= path)
			break;
		if (*trv == '/') {
			*trv = '\0';
			if (stat(path, &sbuf))
				return (0);
			if (!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				return (0);
			}
			*trv = '/';
			break;
		}
	}

	for (;;) {
		if (doopen) {
			if ((*doopen =
			    open(path, O_CREAT | O_EXCL | O_RDWR, 0600)) >= 0)
				return (1);
			if (errno != EEXIST)
				return (0);
		} else if (domkdir) {
			if (mkdir(path, 0700) >= 0)
				return (1);
			if (errno != EEXIST)
				return (0);
		} else if (lstat(path, &sbuf))
			return (errno == ENOENT ? 1 : 0);

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv)
				return (0);
			if (*trv == 'z')
				*trv++ = 'a';
			else {
				if (isdigit((unsigned char)*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
	/*NOTREACHED*/
}

#define	YES	1
#define	NO	0

int
_gettemp(as, doopen, domkdir)
	char	*as;
	register int *doopen;
	int domkdir;
{
	register char	*start, *trv;
	struct stat	sbuf;
	u_int	pid;

	_DIAGASSERT(as != NULL);

	pid = getpid();

	/* extra X's get set to 0's */
	for (trv = as; *trv; ++trv);
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	/*
	 * check for write permission on target directory; if you have
	 * six X's and you can't write the directory, this will run for
	 * a *very* long time.
	 */
	for (start = ++trv; trv > as && *trv != '/'; --trv);
	if (*trv == '/') {
		*trv = '\0';
		if (stat(as, &sbuf) || !(sbuf.st_mode & S_IFDIR)) {
			return (NO);
		}
		*trv = '/';
	} else if (stat(".", &sbuf) == -1) {
		return (NO);
	}

	for (;;) {
		if (doopen) {
			if ((*doopen = open(as, O_CREAT|O_EXCL|O_RDWR, 0600)) >= 0) {
				return (YES);
			}
			if (errno != EEXIST) {
				return (NO);
			}
		}  else if (domkdir) {
			if (mkdir(as, 0700) >= 0) {
				return (YES);
			}
			if (errno != EEXIST) {
				return (NO);
			}
		} else if (stat(as, &sbuf)) {
			return (errno == ENOENT ? YES : NO);
		}

		/* tricky little algorithm for backward compatibility */
		for (trv = start;;) {
			if (!*trv) {
				return(NO);
			}
			if (*trv == 'z') {
				*trv++ = 'a';
			} else {
				if (isdigit(*trv)) {
					*trv = 'a';
				} else {
					++*trv;
				}
				break;
			}
		}
	}
	/*NOTREACHED*/
}

#endif /* !HAVE_NBTOOL_CONFIG_H || !HAVE_MKSTEMP || !HAVE_MKDTEMP */
