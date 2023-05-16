#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)execvp.c	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#include <sys/param.h>
#include <sys/types.h>
#include <sys/errno.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <paths.h>

/*
 *	execlp(name, arg,...,0)	(like execl, but does path search)
 *	execvp(name, argv)	(like execv, but does path search)
 */
#include <errno.h>

static	char shell[] =	"/bin/sh";
char	*execat(char *, char *, char *);
extern	errno;

int
execlp(name, argv)
	char *name, *argv;
{
	return (execvp(name, &argv));
}

int
execvp(name, argv)
	char *name, **argv;
{
	char *pathstr;
	register char *cp;
	char fname[128];
	char *newargs[256];
	int i;
	register unsigned etxtbsy = 1;
	register eacces = 0;

	if ((pathstr = getenv("PATH")) == NULL)
		pathstr = ":/bin:/usr/bin";
	cp = index(name, '/') ? "" : pathstr;

	do {
		cp = execat(cp, name, fname);

retry: 	execv(fname, argv);
		switch (errno) {
		case ENOEXEC:
			newargs[0] = "sh";
			newargs[1] = fname;
			for (i = 1; newargs[i + 1] == argv[i]; i++) {
				if (i >= 254) {
					errno = E2BIG;
					return (-1);
				}
			}
			execv(shell, newargs);
			return (-1);
		case ETXTBSY:
			if (++etxtbsy > 5)
				return (-1);
			sleep(etxtbsy);
			goto retry;
		case EACCES:
			eacces++;
			break;
		case ENOMEM:
		case E2BIG:
			return (-1);
		}
	} while (cp);
	if (eacces)
		errno = EACCES;
	return (-1);
}

static char *
execat(s1, s2, si)
	register char *s1, *s2;
	char *si;
{
	register char *s;

	s = si;
	while (*s1 && *s1 != ':')
		*s++ = *s1++;
	if (si != s)
		*s++ = '/';
	while (*s2)
		*s++ = *s2++;
	*s = '\0';
	return(*s1? ++s1: 0);
}


extern char **environ;

static char **
buildargv(ap, arg, envpp)
	va_list ap;
	const char *arg;
	char ***envpp;
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
				argv[0] = (char *)arg;
				off = 1;
			}
		}
		if (!(argv[off] = va_arg(ap, char *)))
			break;
	}
	/* Get environment pointer if user supposed to provide one. */
	if (envpp)
		*envpp = va_arg(ap, char **);
	return (argv);
}

int
execl(const char *name, const char *arg, ...)
{
	va_list ap;
	int sverrno;
	char **argv;

	va_start(ap, arg);

	if (argv == buildargv(ap, arg, NULL))
		(void)execve(name, argv, environ);
	va_end(ap);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	return (-1);
}
/*
int
execl(const char *name, const char *arg, ...)
{
	va_list ap;
	int sverrno;
	char **argv;

	va_start(ap, arg);

	if (argv == buildargv(ap, arg, NULL))
		(void)execve(name, argv, environ);
	va_end(ap);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	return (-1);
}
*/

int
execle(const char *name, const char *arg, ...)
{
	va_list ap;
	int sverrno;
	char **argv, **envp;

	va_start(ap, arg);

	if (argv == buildargv(ap, arg, &envp))
		(void)execve(name, argv, envp);
	va_end(ap);
	sverrno = errno;
	free(argv);
	errno = sverrno;
	return (-1);
}
