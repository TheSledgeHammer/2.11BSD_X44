/*
 * Copyright (c) 1989, 1993
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

#include <sys/cdefs.h>
#if !defined(BUILTIN) && !defined(SHELL)
#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1989, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */
#endif

#ifndef lint
#if 0
static char sccsid[] = "@(#)printf.c	8.2 (Berkeley) 3/22/95";
#endif
#endif /* not lint */

#include <sys/types.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef BUILTIN		 /* csh builtin */
#define main progprintf
#endif /* BUILTIN */

#ifdef SHELL		/* sh (aka ash) builtin */
#define main printfcmd
#include "../../bin/sh/bltin/bltin.h"
#endif /* SHELL */

#define PF(f, func) { \
	if (fieldwidth) \
		if (precision) \
			(void)printf(f, fieldwidth, precision, func); \
		else \
			(void)printf(f, fieldwidth, func); \
	else if (precision) \
		(void)printf(f, precision, func); \
	else \
		(void)printf(f, func); \
}

static char *mklong(const char *, char);
static void escape(char *);
static int getchr(void);
static char *getstr(void);
static int getint(int *);
static int getlong(long *);
static int getulong(unsigned long *);
static int getdouble(double *);
static int getnum(intmax_t *, uintmax_t *, int);
static int getfloating(double *, long double *, int);
static int asciicode(void);
static void usage(void);
static void pf_warnx(const char *fmt, ...);

static char **gargv;

int main(int, char **);
int
main(int argc, char *argv[])
{
	int ch, end, fieldwidth, precision;
	char convch, nextch, *format, *start;
	char *fmt;

#if !defined(SHELL) && !defined(BUILTIN)
	(void)setlocale (LC_ALL, "");
#endif

	while ((ch = getopt(argc, argv, "")) != EOF) {
		switch (ch) {
		case '?':
		default:
			usage();
			return (1);
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage();
		return (1);
	}

	/*
	 * Basic algorithm is to scan the format string for conversion
	 * specifications -- once one is found, find out if the field
	 * width or precision is a '*'; if it is, gather up value.  Note,
	 * format strings are reused as necessary to use up the provided
	 * arguments, arguments of zero/null string are provided to use
	 * up the format string.
	 */

#define SKIP1	"#-+ 0"
#define SKIP2	"*0123456789"

	escape(fmt = format = *argv);		/* backslash interpretation */
	gargv = ++argv;
	for (;;) {
		end = 0;
		/* find next format specification */
next:
		for (start = fmt;; ++fmt) {
			if (!*fmt) {
				/* avoid infinite loop */
				if (end == 1) {
					pf_warnx("missing format character");
					return (1);
				}
				end = 1;
				if (fmt > start) {
					(void)printf("%s", start);
				}
				if (!*gargv) {
					return (0);
				}
				fmt = format;
				goto next;
			}
			/* %% prints a % */
			if (*fmt == '%') {
				if (*++fmt != '%') {
					break;
				}
				*fmt++ = '\0';
				(void)printf("%s", start);
				goto next;
			}
		}
		/* skip to field width */
		fmt += strspn(fmt, SKIP1);
		if (*fmt == '*') {
			if (getint(&fieldwidth)) {
				return (1);
			}
		} else {
			fieldwidth = 0;
		}
		/* skip to possible '.', get following precision */
		fmt += strspn(fmt, SKIP2);
		if (*fmt == '.') {
			++fmt;
		}
		if (*fmt == '*') {
			if (getint(&precision)) {
				return (1);
			}
		} else {
			precision = 0;
		}
		/* skip to conversion char */
		fmt += strspn(fmt, SKIP2);
		if (!*fmt) {
			pf_warnx("missing format character");
			return (1);
		}

		convch = *fmt;
		nextch = *++fmt;
		*fmt = '\0';
		switch (convch) {
		case 'b': {
			char *p;

			/* Convert "b" to "s" for output. */
			start[strlen(start) - 1] = 's';
			p = strdup(getstr());
			if (p == NULL) {
				pf_warnx("%s", strerror(ENOMEM));
				return (1);
			}
			escape(p);
        
			PF(start, p);
			/* Restore format for next loop. */
			//free(p);
			break;
		}
		case 'c': {
			char p;

			p = (char)getchr();
			PF(start, p);
			break;
		}
		case 's': {
			char *p;

			p = getstr();
			PF(start, p);
			break;
		}
		case 'd':
		case 'i': {
			long p;
			char *f;

			f = mklong(start, convch);
			if (f == NULL) {
				return (1);
			}
			if (getlong(&p)) {
				return (1);
			}
			PF(f, p);
			break;
		}
		case 'o':
		case 'u':
		case 'x':
		case 'X': {
			unsigned long p;
			char *f;

			f = mklong(start, convch);
			if (f == NULL) {
				return (1);
			}
			if (getulong(&p)) {
				return (1);
			}
			PF(f, p);
			break;
		}
		case 'a':
		case 'A':
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		case 'G': {
			double p;

			if (getdouble(&p)) {
				return (1);
			}
			PF(start, p);
			break;
		}
		default:
			pf_warnx("illegal format character");
			return (1);
		}
		*fmt = nextch;
	}
	/* NOTREACHED */
}

static char *
mklong(const char *str, char ch)
{
	static char copy[64];
	size_t len;

	if (ch == 'X') {
		ch = 'x';
	}
	len = strlen(str) + 2;
	if (len > sizeof(copy)) {
		pf_warnx("format %s too complex\n", str);
		len = 4;
	}
	(void)memmove(copy, str, len - 3);
	copy[len - 3] = 'l';
	copy[len - 2] = ch;
	copy[len - 1] = '\0';
	return (copy);
}

static void
escape(char *fmt)
{
	char *store, c;
	int value;

	for (store = fmt; (c = *fmt) != '\0'; ++fmt, ++store) {
		if (c != '\\') {
			*store = c;
			continue;
		}
		switch (*++fmt) {
		case '\0': 		/* EOS, user error */
			*store = '\\';
			*++store = '\0';
			return;
		case '\\': 		/* backslash */
		case '\'': 		/* single quote */
			*store = *fmt;
			break;
		case 'a': 		/* bell/alert */
			*store = '\a';
			break;
		case 'b': 		/* backspace */
			*store = '\b';
			break;
		case 'e':		/* escape */
#ifdef __GNUC__
			*store = '\e';
#else
			*store = 033;
#endif
			break;
		case 'f': 		/* form-feed */
			*store = '\f';
			break;
		case 'n': 		/* newline */
			*store = '\n';
			break;
		case 'r': 		/* carriage-return */
			*store = '\r';
			break;
		case 't': 		/* horizontal tab */
			*store = '\t';
			break;
		case 'v': 		/* vertical tab */
			*store = '\v';
			break;
			/* octal constant */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
			for (c = 3, value = 0; c-- && *fmt >= '0' && *fmt <= '7'; ++fmt) {
				value <<= 3;
				value += *fmt - '0';
			}
			--fmt;
			*store = (char)value;
			break;
		default:
			*store = *fmt;
			break;
		}
	}
	*store = '\0';
}

static int
getchr(void)
{
	if (!*gargv) {
		return ('\0');
	}
	return ((int)**gargv++);
}

static char *
getstr(void)
{
	static char empty[] = "";
	if (!*gargv) {
		return (empty);
	}
	return (*gargv++);
}

static int
getint(int *ip)
{
	long val = 0;

	if (getlong(&val)) {
		return (1);
	}
	if (val < INT_MIN || val > INT_MAX) {
		pf_warnx("%s: %s", *gargv, strerror(ERANGE));
		return (1);
	}
	*ip = (int)val;
	return (0);
}

static int
getlong(long *lp)
{
	intmax_t val = 0;
	uintmax_t uval = 0;

	if (getnum(&val, &uval, 1)) {
		return (1);
	}
	if (val < LONG_MIN || val > LONG_MAX) {
		pf_warnx("%s: %s", *gargv, strerror(ERANGE));
		return (1);
	}
	*lp = (long)val;
	return (0);
}

static int
getulong(unsigned long *ulp)
{
	intmax_t val = 0;
	uintmax_t uval = 0;

	if (getnum(&val, &uval, 0)) {
		return (1);
	}
	if (uval > ULONG_MAX) {
		pf_warnx("%s: %s", *gargv, strerror(ERANGE));
		return (1);
	}
	*ulp = (unsigned long)uval;
	return (0);
}

static int
getdouble(double *dp)
{
	double val = 0.0;
	long double lval = 0.0;

	if (getfloating(&val, &lval, 1)) {
		return (1);
	}

	if (val < DBL_MIN || val > DBL_MAX) {
		pf_warnx("%s: %s", *gargv, strerror(ERANGE));
		return (1);
	}
	*dp = val;
	return (0);
}

static char Number[] = "+-.0123456789";

static int
getnum(intmax_t *lp, uintmax_t *ulp, int signedconv)
{
	char *ep;

	if (!*gargv) {
		*lp = 0;
		*ulp = 0;
		return (0);
	}
	if (asciicode()) {
		if (signedconv) {
			*lp = (intmax_t)asciicode();
		} else {
			*ulp = (uintmax_t)asciicode();
		}
		return (0);
	}
	ep = strchr(Number, **gargv);
	if (ep) {
		errno = 0;
		if (signedconv) {
			*lp = strtoimax(*gargv, &ep, 0);
		} else {
			*ulp = strtoumax(*gargv, &ep, 0);
		}
		if (*ep != '\0') {
			pf_warnx("%s: illegal number", *gargv);
			return (1);
		}
		if (errno == ERANGE) {
			pf_warnx("%s: %s", *gargv, strerror(ERANGE));
			return (1);
		}
		++gargv;
	}
	return (0);
}

static int
getfloating(double *dp, long double *ldp, int mod_ldbl)
{
	char *ep;

	if (!*gargv) {
		*dp = 0.0;
		*ldp = 0.0;
		return (0);
	}
	if (asciicode()) {
		if (mod_ldbl) {
			*dp = (double)asciicode();
		} else {
			*ldp = (long double)asciicode();
		}
		return (0);
	}
	ep = strchr(Number, **gargv);
	if (ep) {
		errno = 0;
		if (mod_ldbl) {
			*dp = strtod(*gargv, &ep);
		} else {
			*ldp = strtold(*gargv, &ep);
		}
		if (*ep != '\0') {
			pf_warnx("%s: illegal number", *gargv);
			return (1);
		}
		if (errno == ERANGE) {
			pf_warnx("%s: %s", *gargv, strerror(ERANGE));
			return (1);
		}
		++gargv;
	}
	return (0);
}

static int
asciicode(void)
{
	int ch;

	ch = **gargv;
	if (ch == '\'' || ch == '\"') {
		ch = (*gargv)[1];
	}
	++gargv;
	return (ch);
}

/*
 * Workaround for csh progprinf.
 * Not liking warnx.
 */
static void
pf_warnx(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
#if !defined(SHELL)
	if (fmt != NULL) {
		(void)doprnt(stderr, fmt, ap);
	}
#else
    vwarnx(fmt, ap);
#endif
	va_end(ap);
}

static void
usage(void)
{
	(void)fprintf(stderr, "usage: printf format [arg ...]\n");
}
