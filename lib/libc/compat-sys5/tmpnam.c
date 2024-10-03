/*-
 * Copyright (c) 1990, 1993, 1994
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
static char sccsid[] = "@(#)tmpnam.c	4.6 (Berkeley) 7/23/88";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <paths.h>

FILE *
#if __STDC__
tmpfile(void)
#else
tmpfile()
#endif
{
	sigset_t set, oset;
	FILE *fp;
	int fd, sverrno;
#define	TRAILER	"tmp.XXXXXX"
	char buf[sizeof(_PATH_TMP) + sizeof(TRAILER)];

	(void)memcpy(buf, _PATH_TMP, sizeof(_PATH_TMP) - 1);
	(void)memcpy(buf + sizeof(_PATH_TMP) - 1, TRAILER, sizeof(TRAILER));

	sigfillset(&set);
	(void)sigprocmask(SIG_BLOCK, &set, &oset);

	fd = mkstemp(buf);
	if (fd != -1)
		(void)unlink(buf);

	(void)sigprocmask(SIG_SETMASK, &oset, NULL);

	if (fd == -1)
		return (NULL);

	if ((fp = fdopen(fd, "w+")) == NULL) {
		sverrno = errno;
		(void)close(fd);
		errno = sverrno;
		return (NULL);
	}
	return (fp);
}

char *
#if __STDC__
tmpnam(char *s)
#else
tmpnam(s)
	char *s;
#endif
{
	static u_long tmpcount;
	static char buf[L_tmpnam];

    if (s == NULL) {
        s = buf;
    }
	(void)snprintf(s, L_tmpnam, "%stmp.%lu.XXXXXX", P_tmpdir, tmpcount);
    ++tmpcount;
	return (mktemp(s));
}

static char *
#if __STDC__
gentemp(char *name, size_t len, const char *tmp, const char  *pfx)
#else
gentemp(name, len, tmp, pfx)
    char *name;
    size_t len;
    const char *tmp, *pfx;
#endif
{
    const char *ftmp = *(tmp + strlen(tmp) - 1) == '/'? "": "/";
    (void)snprintf(name, len, "%s%s%sXXXXXX", tmp, ftmp, pfx);
    return (mktemp(name));
}

char *
#if __STDC__
tempnam(const char *dir, const char *pfx)
#else
tempnam(dir, pfx)
	const char *dir, *pfx;
#endif
{
	int sverrno;
	char *f, *name;
    const char *tmp;

	if (!(name = malloc(MAXPATHLEN)))
		return (NULL);

	if (!pfx)
		pfx = "tmp.";

	if ((tmp = getenv("TMPDIR")))  {
        f = gentemp(name, (size_t)MAXPATHLEN, tmp, pfx);
        if (f != NULL) {
            return (f);
        }
	}

	if (dir != NULL) {
        f = gentemp(name, (size_t)MAXPATHLEN, dir, pfx);
        if (f != NULL) {
            return (f);
        }
	}

    f = gentemp(name, (size_t)MAXPATHLEN, P_tmpdir, pfx);
    if (f != NULL) {
        return (f);
    }

    f = gentemp(name, (size_t)MAXPATHLEN, _PATH_TMP, pfx);
    if (f != NULL) {
        return (f);
    }

	sverrno = errno;
	free(name);
	errno = sverrno;
	return (NULL);
}
