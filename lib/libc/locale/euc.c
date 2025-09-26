/*	$NetBSD: citrus_euc.c,v 1.17 2014/01/18 15:21:41 christos Exp $	*/

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
static char sccsid[] = "@(#)euc.c	8.1 (Berkeley) 6/4/93";
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

#include <citrus/citrus_types.h>
#include <citrus/citrus_ctype.h>
#include <citrus/citrus_stdenc.h>

#include "setlocale.h"

typedef _Encoding_Info				_EUCEncodingInfo;
typedef _Encoding_TypeInfo 			_EUCCTypeInfo;
typedef _Encoding_State				_EUCState;

#define	_SS2						0x008e
#define	_SS3						0x008f

#define _FUNCNAME(m)				_EUC_##m

rune_t	_EUC_sgetrune(const char *, size_t, char const **);
int		_EUC_sputrune(rune_t, char *, size_t, char **);
int		_EUC_sgetmbrune(_EUCEncodingInfo * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _EUCState * __restrict, size_t * __restrict);
int 	_EUC_sputmbrune(_EUCEncodingInfo * __restrict, char * __restrict, size_t, wchar_t, _EUCState * __restrict, size_t * __restrict);
int		_EUC_sgetcsrune(_EUCEncodingInfo * __restrict, wchar_t * __restrict, _csid_t, _index_t);
int		_EUC_sputcsrune(_EUCEncodingInfo * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);
int 	_EUC_module_init(_EUCEncodingInfo * __restrict, const void * __restrict, size_t);
void	_EUC_module_uninit(_EUCEncodingInfo *);

static int _EUC_set(u_int);

_RuneOps _euc_runeops = {
		.ro_sgetrune 	=  	_EUC_sgetrune,
		.ro_sputrune 	=  	_EUC_sputrune,
		.ro_sgetmbrune 	=  	_EUC_sgetmbrune,
		.ro_sputmbrune 	=  	_EUC_sputmbrune,
		.ro_sgetcsrune  =	_EUC_sgetcsrune,
		.ro_sputcsrune	= 	_EUC_sputcsrune,
		.ro_module_init = 	_EUC_module_init,
		.ro_module_uninit = _EUC_module_uninit,
};

int
_EUC_init(rl)
	_RuneLocale *rl;
{
	int ret;

	rl->ops = &_euc_runeops;

	ret = _citrus_ctype_init((void **)&rl, rl->variable, rl->variable_len);
	if (ret != 0) {
		return (ret);
	}
	ret = _citrus_stdenc_init((void **)&rl, rl->variable, rl->variable_len);
	if (ret != 0) {
		return (ret);
	}
	rl->variable_len = sizeof(_EUCEncodingInfo);
	_CurrentRuneLocale = rl;

	return (0);
}

int
_EUC_sgetmbrune(_EUCEncodingInfo * __restrict ei, wchar_t * __restrict pwc, const char ** __restrict s, size_t n, _EUCState * __restrict psenc, size_t *__restrict nresult)
{
	wchar_t wchar;
	int c, cs, len;
	int chlenbak;
	const char *s0, *s1 = NULL;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s != NULL);

	s0 = *s;

	if (s0 == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0; /* state independent */
		return (0);
	}

	chlenbak = psenc->chlen;

	/* make sure we have the first byte in the buffer */
	switch (psenc->chlen) {
	case 0:
		if (n < 1) {
			goto restart;
		}
		psenc->ch[0] = *s0++;
		psenc->chlen = 1;
		n--;
		break;
	case 1:
	case 2:
		break;
	default:
		/* illgeal state */
		goto encoding_error;
	}

	c = ei->count[cs = _EUC_set(psenc->ch[0] & 0xff)];
	if (c == 0) {
		goto encoding_error;
	}
	while (psenc->chlen < c) {
		if (n < 1) {
			goto restart;
		}
		psenc->ch[psenc->chlen] = *s0++;
		psenc->chlen++;
		n--;
	}
	*s = s0;

	switch (cs) {
	case 3:
	case 2:
		/* skip SS2/SS3 */
		len = c - 1;
		s1 = (const char *)&psenc->ch[1];
		break;
	case 1:
	case 0:
		len = c;
		s1 = (const char *)&psenc->ch[0];
		break;
	default:
		goto encoding_error;
	}
	wchar = 0;
	while (len-- > 0) {
		wchar = (wchar << 8) | (*s1++ & 0xff);
	}
	wchar = (wchar & ~ei->mask) | ei->bits[cs];

	psenc->chlen = 0;
	if (pwc) {
		*pwc = wchar;
	}

	if (!wchar) {
		*nresult = 0;
	} else {
		*nresult = (size_t)(c - chlenbak);
	}

	return 0;

encoding_error:
	psenc->chlen = 0;
	*nresult = (size_t) - 1;
	return (EILSEQ);

restart:
	*nresult = (size_t) - 2;
	*s = s0;
	return (0);
}

int
_EUC_sputmbrune(_EUCEncodingInfo * __restrict ei, char * __restrict s, size_t n, wchar_t wc, _EUCState * __restrict psenc, size_t *__restrict nresult)
{
	wchar_t m, nm;
	int cs, i, ret;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	m = wc & ei->mask;
	nm = wc & ~m;

	for (cs = 0; cs < sizeof(ei->count) / sizeof(ei->count[0]); cs++) {
		if (m == ei->bits[cs])
			break;
	}
	/* fallback case - not sure if it is necessary */
	if (cs == sizeof(ei->count) / sizeof(ei->count[0])) {
		cs = 1;
	}

	i = ei->count[cs];
	if (n < i) {
		ret = E2BIG;
		goto err;
	}
	m = (cs) ? 0x80 : 0x00;
	switch (cs) {
	case 2:
		*s++ = _SS2;
		i--;
		break;
	case 3:
		*s++ = _SS3;
		i--;
		break;
	}

	while (i-- > 0) {
		*s++ = ((nm >> (i << 3)) & 0xff) | m;
	}

	*nresult = (size_t) ei->count[cs];
	return 0;

err:
	*nresult = (size_t)-1;
	return ret;
}

rune_t
_EUC_sgetrune(const char *string, size_t n, char const **result)
{
	return (emulated_sgetrune(string, n, result));
}

int
_EUC_sputrune(rune_t c, char *string, size_t n, char **result)
{
	return (emulated_sputrune(c, string, n, result));
}

int
_EUC_sgetcsrune(_EUCEncodingInfo * __restrict ei, wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{

	_DIAGASSERT(ei != NULL && wc != NULL);

	if ((csid & ~ei->mask) != 0 || (idx & ei->mask) != 0) {
		return (EINVAL);
	}

	*wc = (wchar_t)csid | (wchar_t)idx;

	return (0);
}

int
_EUC_sputcsrune(_EUCEncodingInfo * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
	wchar_t m, nm;

	_DIAGASSERT(ei != NULL && csid != NULL && idx != NULL);

	m = wc & ei->mask;
	nm = wc & ~m;

	*csid = (_citrus_csid_t)m;
	*idx  = (_citrus_index_t)nm;

	return (0);
}

static int
_EUC_set(u_int c)
{
	c &= 0xff;

	return ((c & 0x80) ? c == _SS3 ? 3 : c == _SS2 ? 2 : 1 : 0);
}

static int
parse_variable(_EUCEncodingInfo *ei, const void *var, size_t lenvar)
{
	const char *v, *e;
	int x;

	/* parse variable string */
	if (!var) {
		return (EFTYPE);
	}

	v = (const char *)var;
	while (*v == ' ' || *v == '\t') {
		++v;
	}

	ei->mb_cur_max = 1;
	for (x = 0; x < 4; ++x) {
		ei->count[x] = (int)strtol(v, __UNCONST(&e), 0);
		if (v == e || !(v = e) || ei->count[x] < 1 || ei->count[x] > 4) {
			return (EFTYPE);
		}
		if (ei->mb_cur_max < ei->count[x])
			ei->mb_cur_max = ei->count[x];
		while (*v == ' ' || *v == '\t') {
			++v;
		}
		ei->bits[x] = (int)strtol(v, __UNCONST(&e), 0);
		if (v == e || !(v = e)) {
			return (EFTYPE);
		}
		while (*v == ' ' || *v == '\t') {
			++v;
		}
	}
	ei->mask = (int)strtol(v, __UNCONST(&e), 0);
	if (v == e || !(v = e)) {
		return (EFTYPE);
	}

	return 0;
}

int
_EUC_module_init(_EUCEncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
	_DIAGASSERT(ei != NULL);

	return (parse_variable(ei, var, lenvar));
}

void
_EUC_module_uninit(_EUCEncodingInfo *ei)
{

}
