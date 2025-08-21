/*-
 * Copyright (c) 1991, 1993
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
static char sccsid[] = "@(#)execvp.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <paths.h>

#if __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef __weak_alias
__weak_alias(execl,_execl)
__weak_alias(execle,_execle)
__weak_alias(execlp,_execlp)
//__weak_alias(execv,_execv)
__weak_alias(execvp,_execvp)
__weak_alias(execvpe,_execvpe)
#endif

/*
 *	execlp(name, arg,...,0)	(like execl, but does path search)
 *	execvp(name, argv)	(like execv, but does path search)
 */

extern char **environ;
static char *execat(const char *, char *, char *);
static char **buildargv(va_list, const char *, char ***);

int
execl(const char *name, const char *arg, ...)
{
	va_list ap;
	int sverrno, error;
	char **argv;

    error = -1;
	va_start(ap, arg);

	if ((argv = buildargv(ap, arg, NULL))) {
		error = execve(name, argv, environ);
	}
	va_end(ap);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	return (error);
}

int
execle(const char *name, const char *arg, ...)
{
	va_list ap;
	int sverrno, error;
	char **argv, **envp;

    error = -1;
	va_start(ap, arg);

	if ((argv = buildargv(ap, arg, &envp))) {
		error = execve(name, argv, envp);
	}
	va_end(ap);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	return (error);
}

int
execlp(const char *name, const char *arg, ...)
{
	va_list ap;
	int sverrno, error;
	char **argv;

    error = -1;
	va_start(ap, arg);
	if ((argv = buildargv(ap, arg, NULL))) {
		error = execvp(name, argv);
	}
	va_end(ap);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	return (error);
}

/*
int
execv(const char *name, char * const *argv)
{
	(void)execve(name, argv, environ);
	return (-1);
}
*/

int
execvpe(const char *name, char * const *argv, char * const *envp)
{
	static int memsize;
	static const char **newargs;
	register int cnt;
	register char *cp;
	int eacces, etxtbsy;
	char buf[MAXPATHLEN];
	const char *fname, *cur, *pathstr;
	etxtbsy = 1;
	eacces = 0;

	/* If it's an absolute or relative path name, it's easy. */
	if (index(name, '/')) {
		fname = name;
		cur = pathstr = NULL;
		goto retry;
	}
	fname = buf;

	/* Get the path we're searching. */
	if (!(pathstr = getenv("PATH"))) {
		pathstr = _PATH_DEFPATH;
	}
	cur = pathstr = __UNCONST(strdup(pathstr));

	while ((cp = strsep(__UNCONST(&cur), ":"))) {
		cp = execat(name, cp, buf);
retry:
		(void)execve(fname, argv, envp);
		switch (errno) {
		case ENOEXEC:
			for (cnt = 0; argv[cnt]; ++cnt);
			if ((cnt + 2) * sizeof(char *) > memsize) {
				memsize = (cnt + 2) * sizeof(char *);
				if ((newargs = realloc(newargs, memsize)) == NULL) {
					memsize = 0;
					goto done;
				}
			}
			newargs[0] = "sh";
			newargs[1] = fname;
			bcopy(argv + 1, newargs + 2, cnt * sizeof(char *));
			(void)execve(_PATH_BSHELL, __UNCONST(newargs), envp);
			return (-1);
		case ETXTBSY:
			if (++etxtbsy > 5) {
				return (-1);
			}
			sleep(etxtbsy);
			goto retry;
		case EACCES:
			eacces++;
			break;
		case ENOENT:
			break;
		case ENOMEM:
		case ENOTDIR:
		case EISDIR:
		case ELOOP:
		case ENAMETOOLONG:
		case E2BIG:
			goto done;
		default:
			goto done;
		}
	}
	if (eacces) {
		errno = EACCES;
	} else if (!errno) {
		errno = ENOENT;
	}
done:
	if (pathstr) {
		free(__UNCONST(pathstr));
	}
	return (-1);
}

int
execvp(const char *name, char * const *argv)
{
	return (execvpe(name, argv, environ));
}

static char *
execat(const char *s1, char *si, char *buf)
{
    register int lp, ln;

	if (!*si) {
		si = __UNCONST(".");
		lp = 1;
	} else {
		lp = strlen(si);
	}
    ln = strlen(s1);

    if (lp + ln + 2 > sizeof(buf)) {
		(void) write(STDERR_FILENO, "execvp: ", 8);
		(void) write(STDERR_FILENO, si, lp);
		(void) write(STDERR_FILENO, ": path too long\n", 16);
    }

	bcopy(si, buf, lp);
	buf[lp] = '/';
	bcopy(s1, buf + lp + 1, ln);
	buf[lp + ln + 1] = '\0';
    return (si);
}

static char **
buildargv(va_list ap, const char *arg, char ***envpp)
{
	static int memsize;
	static char **argv;
	register int off;

	argv = NULL;
	for (off = 0;; ++off) {
		if (off >= memsize) {
			memsize += 50;	/* Starts out at 0. */
			memsize *= 2;	/* Ramp up fast. */
			if (!(argv = realloc(argv, memsize * sizeof(char *)))) {
				memsize = 0;
				return (NULL);
			}
			if (off == 0) {
				argv[0] = __UNCONST(arg);
				off = 1;
			}
		}
		if (!(argv[off] = va_arg(ap, char *))) {
			break;
		}
	}
	/* Get environment pointer if user supposed to provide one. */
	if (envpp) {
		*envpp = va_arg(ap, char **);
	}
	return (argv);
}
