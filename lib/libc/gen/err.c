/*-
 * Copyright (c) 1993
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

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)err.c	8.1.1 (2.11BSD GTE) 2/3/95";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

static	void putprog(void);
static 	void putcolsp(void);

#ifdef __weak_alias
__weak_alias(err, _err)
__weak_alias(errc, _errc)
__weak_alias(errx, _errx)
__weak_alias(verrx, _verrx)
__weak_alias(verr, _verr)
__weak_alias(verrc, _verrc)
__weak_alias(verrx, _verrx)
__weak_alias(vwarn, _vwarn)
__weak_alias(vwarnc, _vwarnc)
__weak_alias(vwarnx, _vwarnx)
__weak_alias(warn, _warn)
__weak_alias(warnc, _warnc)
__weak_alias(warnx, _warnx)
#endif

#if !HAVE_DECL_ERR
void
err(int eval, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verr(eval, fmt, ap);
	va_end(ap);
}
#endif

#if !HAVE_DECL_ERRC
void
errc(int eval, int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	verrc(eval, code, fmt, ap);
	va_end(ap);
}
#endif

#if !HAVE_DECL_ERRX
void
errx(int eval, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrx(eval, fmt, ap);
	va_end(ap);
}
#endif

#if !HAVE_DECL_VERR
void
verr(int eval, const char *fmt, va_list ap)
{
	int sverrno;

	sverrno = errno;
	putprog();
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		putcolsp();
	}
	(void)fputs(strerror(sverrno), stderr);
	(void)fputc('\n', stderr);
	exit(eval);
}
#endif

#if !HAVE_DECL_VERRC
void
verrc(int eval, int code, const char *fmt, va_list ap)
{
	(void)fprintf(stderr, "%s: ", getprogname());
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(code));
	exit(eval);
}
#endif

#if !HAVE_DECL_VERRX
void
verrx(int eval, const char *fmt, va_list ap)
{
	putprog();
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fputc('\n', stderr);
	exit(eval);
}
#endif

#if !HAVE_DECL_VWARN
void
vwarn(const char *fmt, va_list ap)
{
	int sverrno;

	sverrno = errno;
	putprog();
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		putcolsp();
	}
	(void)fputs(strerror(sverrno), stderr);
	(void)fputc('\n', stderr);
}
#endif

#if !HAVE_DECL_VWARNC
void
vwarnc(int code, const char *fmt, va_list ap)
{
	(void)fprintf(stderr, "%s: ", getprogname());
	if (fmt != NULL) {
		(void)vfprintf(stderr, fmt, ap);
		(void)fprintf(stderr, ": ");
	}
	(void)fprintf(stderr, "%s\n", strerror(code));
}
#endif

#if !HAVE_DECL_VWARNX
void
vwarnx(const char *fmt, va_list ap)
{
	putprog();
	if (fmt != NULL)
		(void)vfprintf(stderr, fmt, ap);
	(void)fputc('\n', stderr);
}
#endif

#if !HAVE_DECL_WARN
void
warn(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarn(fmt, ap);
	va_end(ap);
}
#endif

#if !HAVE_DECL_WARNC
void
warnc(int code, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarnc(code, fmt, ap);
	va_end(ap);
}
#endif

#if !HAVE_DECL_WARNX
void
warnx(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
}
#endif

/*
 * Helper routines.  Repeated constructs of the form "%s: " used up too
 * much D space.  On a pdp-11 code can be overlaid but Data space is worth
 * conserving.  An extra function call or two handling an error condition is
 * a reasonable trade for 20 or 30 bytes of D space.
*/

static void
putprog(void)
{
	fputs(getprogname(), stderr);
	putcolsp();
}

static void
putcolsp(void)
{
	fputc(':', stderr);
	fputc(' ', stderr);
}
