/*	$NetBSD: sscanf.c,v 1.22 2018/02/04 01:13:45 mrg Exp $	*/

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
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)scanf.c	5.2 (Berkeley) 3/9/86";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#include "reentrant.h"
#include "setlocale.h"
#include "local.h"

__weak_alias(scanf_l, _scanf_l)
__weak_alias(fscanf_l, _fscanf_l)
__weak_alias(vscanf_l, _vscanf_l)
__weak_alias(sscanf_l, _sscanf_l)
__weak_alias(vsscanf_l, _vsscanf_l)

static int eofread(void *, char *, int);

static int
eofread(void *cookie, char *buf, int len)
{
	return (0);
}

int
scanf_l(locale_t locale, char const *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = doscan_l(stdin, locale, fmt, ap);
	va_end(ap);
	return(ret);
}

int
scanf(char const *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = doscan(stdin, fmt, ap);
	va_end(ap);
	return(ret);
}

int
fscanf_l(FILE *fp, locale_t locale, char const *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = doscan_l(stdin, locale, fmt, ap);
	va_end(ap);
	return (ret);
}

int
fscanf(FILE *fp, char const *fmt, ...)
{
	int ret;
	va_list ap;

	va_start(ap, fmt);
	ret = doscan(stdin, fmt, ap);
	va_end(ap);
	return (ret);
}

int
vscanf_l(locale_t locale, const char *fmt, va_list ap)
{
	return (doscan_l(stdin, locale, fmt, ap));
}

int
vscanf(const char *fmt, va_list ap)
{
	return (doscan(stdin, fmt, ap));
}

int
sscanf_l(const char *str, locale_t locale, char const *fmt, ...)
{
	int ret;
	va_list ap;
	FILE _strbuf;
	struct __sfileext fext;

	_DIAGASSERT(str != NULL);

	va_start(ap, fmt);
	_FILEEXT_SETUP(&_strbuf, &fext);
	_strbuf._flags = __SRD|_IOREAD|_IOSTRG;
	_strbuf._bf._base = _strbuf._p = __UNCONST(str);
	_strbuf._bf._size = _strbuf._r = strlen(str);
	_strbuf._read = eofread;
	_strbuf._ub._base = NULL;
	_strbuf._lb._base = NULL;
	ret = doscan_l(&_strbuf, locale, fmt, ap);
	va_end(ap);
	return (ret);
}

int
sscanf(const char *str, char const *fmt, ...)
{
	return (sscanf_l(str, __get_locale(), fmt));
}

int
vsscanf_l(const char *str, locale_t locale, const char *fmt, va_list ap)
{
	int ret;
	FILE _strbuf;
	struct __sfileext fext;

	_DIAGASSERT(str != NULL);

	_FILEEXT_SETUP(&_strbuf, &fext);
	_strbuf._flags = __SRD;
	_strbuf._bf._base = _strbuf._p = __UNCONST(str);
	_strbuf._bf._size = _strbuf._r = strlen(str);
	_strbuf._read = eofread;
	_strbuf._ub._base = NULL;
	_strbuf._lb._base = NULL;
    ret = doscan_l(&_strbuf, locale, fmt, ap);
	return (ret);
}

int
vsscanf(const char *str, const char *fmt, va_list ap)
{
	return (vsscanf_l(str, __get_locale(), fmt, ap));
}
