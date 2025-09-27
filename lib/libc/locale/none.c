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
#if 0
static char sccsid[] = "@(#)none.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <rune.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <limits.h>

#undef _ENCODING_MB_CUR_MAX

#include <citrus/citrus_types.h>
#include <citrus/citrus_ctype.h>
#include <citrus/citrus_stdenc.h>

#include "setlocale.h"

typedef _Encoding_Info				_NONEEncodingInfo;
typedef _Encoding_TypeInfo 			_NONECTypeInfo;
typedef _Encoding_State				_NONEState;

#define _FUNCNAME(m)				_NONE_##m
#define _ENCODING_MB_CUR_MAX(_ei_)	1

rune_t		_NONE_sgetrune(const char *, size_t, char const **);
int			_NONE_sputrune(rune_t, char *, size_t, char **);
int		    _NONE_sgetmbrune(_NONEEncodingInfo * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _NONEState * __restrict, size_t * __restrict);
int 		_NONE_sputmbrune(_NONEEncodingInfo * __restrict, char * __restrict, size_t, wchar_t, _NONEState * __restrict, size_t * __restrict);
int			_NONE_sgetcsrune(_NONEEncodingInfo * __restrict, wchar_t * __restrict, _csid_t, _index_t);
int			_NONE_sputcsrune(_NONEEncodingInfo * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);
int			_NONE_module_init(_NONEEncodingInfo * __restrict, const void * __restrict, size_t);
void		_NONE_module_uninit(_NONEEncodingInfo *);

_RuneOps _none_runeops = {
		.ro_sgetrune 	=  	_NONE_sgetrune,
		.ro_sputrune 	=  	_NONE_sputrune,
		.ro_sgetmbrune 	=  	_NONE_sgetmbrune,
		.ro_sputmbrune 	=  	_NONE_sputmbrune,
		.ro_sgetcsrune  =	_NONE_sgetcsrune,
		.ro_sputcsrune	= 	_NONE_sputcsrune,
		.ro_module_init = 	_NONE_module_init,
		.ro_module_uninit = 	_NONE_module_uninit,
};

int
_NONE_init(rl)
	_RuneLocale *rl;
{
	rl->ops = &_none_runeops;
	rl->variable_len = sizeof(_NONEEncodingInfo);
	_CurrentRuneLocale = rl;

	return (0);
}

int
_NONE_sgetmbrune(_NONEEncodingInfo * __restrict ei, wchar_t * __restrict pwc, const char ** __restrict s, size_t n, _NONEState * __restrict psenc, size_t * __restrict nresult)
{
    const char *s0;

    s0 = *s;
	if (s0 == NULL) {
		*nresult = 0; /* state independent */
		return (0);
	}
	if (n == 0) {
        *nresult = (size_t)-2;
		return (0);
	}
    if (pwc != NULL) {
        *pwc = (wchar_t)s0;
    }
	*nresult = *s0 == '\0' ? 0 : 1;
    *s = s0;
	return (0);
}

int
_NONE_sputmbrune(_NONEEncodingInfo * __restrict ei, char * __restrict s, size_t n, wchar_t wc, _NONEState * __restrict psenc, size_t * __restrict nresult)
{
	if ((wc & ~0xFFU) != 0) {
		*nresult = (size_t) - 1;
		return (EILSEQ);
	}

	*nresult = 1;
	if (s != NULL) {
		*s = (wchar_t) wc;
	}

	return (0);
}

rune_t
_NONE_sgetrune(const char *string, size_t n, char const **result)
{
	return (emulated_sgetrune(string, n, result));
}

int
_NONE_sputrune(rune_t c, char *string, size_t n, char **result)
{
	return (emulated_sputrune(c, string, n, result));
}

int
_NONE_sgetcsrune(_NONEEncodingInfo * __restrict cl, wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{
	return (0);
}

int
_NONE_sputcsrune(_NONEEncodingInfo * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
	return (0);
}

int
_NONE_module_init(_NONEEncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
	return (0);
}

void
_NONE_module_uninit(_NONEEncodingInfo *ei)
{

}
