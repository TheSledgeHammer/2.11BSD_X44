/*	$NetBSD: citrus_none.c,v 1.23 2019/07/28 13:52:23 christos Exp $	*/

/*-
 * Copyright (c)2002 Citrus Project,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*-
 * Copyright (c) 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
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
static char sccsid[] = "@(#)none.c	8.1 (Berkeley) 6/4/93";
#endif /* LIBC_SCCS and not lint */

#include <stddef.h>
#include <stdio.h>
#include "rune.h"
#include <errno.h>
#include <stdlib.h>

#include "citrus_ctype.h"

#define _FUNCNAME(m)				_none_citrus_ctype_##m
#define _ENCODING_MB_CUR_MAX(_ei_)	1

rune_t		_none_sgetrune(const char *, size_t, char const **);
int			_none_sputrune(rune_t, char *, size_t, char **);
int			_none_sgetrune_mb(void * __restrict, wchar_t * __restrict, const char * __restrict, size_t, void * __restrict, size_t * __restrict);
int 		_none_sputrune_mb(void * __restrict, char * __restrict, wchar_t, void * __restrict, size_t * __restrict);

int
_none_init(rl)
	_RuneLocale *rl;
{
	rl->ops->ro_sgetrune = _none_sgetrune;
	rl->ops->ro_sputrune = _none_sputrune;
	rl->ops->ro_sgetrune_mb = _none_sgetrune_mb;
	rl->ops->ro_sputrune_mb = _none_sputrune_mb;

	_CurrentRuneLocale = rl;

	return (0);
}

int
_none_citrus_ctype_mbrtowc_priv(void * __restrict cl, wchar_t * __restrict pwc, const char * __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult)
{
	return (_none_sgetrune_mb(cl, pwc, s, n, pspriv, nresult));
}

int
_none_citrus_ctype_wcrtomb(void * __restrict cl, char * __restrict s, wchar_t wc, void * __restrict pspriv, size_t * __restrict nresult)
{
	return (_none_sputrune_mb(cl, s, wc, pspriv, nresult));
}

int
_none_sgetrune_mb(void * __restrict cl, wchar_t * __restrict pwc, const char * __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult)
{
	if (s == NULL) {
		*nresult = 0;
		return (0);
	}
	if (n == 0) {
		*nresult = (size_t) -2;
		return (0);
	}

	pwc = (wchar_t) _none_sgetrune(s, n, nresult);

	if (pwc != NULL) {
		*pwc = (wchar_t) (unsigned char) *s;
	}

	*nresult = *s == '\0' ? 0 : 1;

	return (0);
}

int
_none_sputrune_mb(void * __restrict cl, char * __restrict s, wchar_t wc, void * __restrict pspriv, size_t * __restrict nresult)
{
	if ((wc & ~0xFFU) != 0) {
		*nresult = (size_t) -1;
		return (_none_sputrune(wc, s, 1, nresult));
	}

	*nresult = 1;
	if (s != NULL) {
		*s = (char) wc;
	}

	return (_none_sputrune(wc, s, 0, nresult));
}

rune_t
_none_sgetrune(string, n, result)
	const char *string;
	size_t n;
	char const **result;
{
	int c;

	if (n < 1) {
		if (result)
			*result = string;
		return (_INVALID_RUNE);
	}
	if (result)
		*result = string + 1;
	return (*string & 0xff);
}

int
_none_sputrune(c, string, n, result)
	rune_t c;
	char *string, **result;
	size_t n;
{
	if (n >= 1) {
		if (string)
			*string = c;
		if (result)
			*result = string + 1;
	} else if (result)
		*result = (char *)0;
	return (1);
}
