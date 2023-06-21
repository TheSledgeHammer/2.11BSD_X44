/*	$NetBSD: citrus_utf8.c,v 1.10 2003/08/07 16:42:38 agc Exp $	*/

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

#include <sys/cdefs.h>

#include <errno.h>
#include <rune.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include <citrus/citrus_ctype.h>
#include <citrus/citrus_stdenc.h>

typedef _Encoding_Info				_UTF8EncodingInfo;
typedef _Encoding_TypeInfo 			_UTF8CTypeInfo;
typedef _Encoding_State				_UTF8State;

#define _FUNCNAME(m)				_UTF8_##m
#define _ENCODING_MB_CUR_MAX(_ei_)	6

rune_t	_UTF8_sgetrune(const char *, size_t, char const **);
int		_UTF8_sputrune(rune_t, char *, size_t, char **);
int		_UTF8_sgetmbrune(_UTF8EncodingInfo *, wchar_t *, const char **, size_t, _UTF8State *, size_t *);
int 	_UTF8_sputmbrune(_UTF8EncodingInfo *, char *, wchar_t, _UTF8State *, size_t *);
int		_UTF8_sgetcsrune(_UTF8EncodingInfo * __restrict, wchar_t * __restrict, _csid_t, _index_t);
int		_UTF8_sputcsrune(_UTF8EncodingInfo * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);

static int _UTF8_count_array[256];
static int const *_UTF8_count = NULL;

struct _RuneOps _utf8_runeops = {
		.ro_sgetrune 	=  	_UTF8_sgetrune,
		.ro_sgetrune 	=  	_UTF8_sgetrune,
		.ro_sgetmbrune 	=  	_UTF8_sgetmbrune,
		.ro_sgetmbrune 	=  	_UTF8_sgetmbrune,
		.ro_sgetcsrune  =	_UTF8_sgetcsrune,
		.ro_sputcsrune	= 	_UTF8_sputcsrune,
};

static u_int32_t _UTF8_range[] = {
	0,	/*dummy*/
	0x00000000, 0x00000080, 0x00000800, 0x00010000,
	0x00200000, 0x04000000, 0x80000000,
};

static int
_UTF8_findlen(wchar_t v)
{
	int i;
	u_int32_t c;

	c = (u_int32_t)v;	/*XXX*/
	for (i = 1; i < sizeof(_UTF8_range) / sizeof(_UTF8_range[0]); i++) {
		if (c >= _UTF8_range[i] && c < _UTF8_range[i + 1]) {
			return (i);
		}
	}

	return (-1);	/*out of range*/
}

static __inline void
_UTF8_init_count(void)
{
	int i;
	if (!_UTF8_count) {
		memset(_UTF8_count_array, 0, sizeof(_UTF8_count_array));
		for (i = 0; i <= 0x7f; i++)
			_UTF8_count_array[i] = 1;
		for (i = 0xc0; i <= 0xdf; i++)
			_UTF8_count_array[i] = 2;
		for (i = 0xe0; i <= 0xef; i++)
			_UTF8_count_array[i] = 3;
		for (i = 0xf0; i <= 0xf7; i++)
			_UTF8_count_array[i] = 4;
		for (i = 0xf8; i <= 0xfb; i++)
			_UTF8_count_array[i] = 5;
		for (i = 0xfc; i <= 0xfd; i++)
			_UTF8_count_array[i] = 6;
		_UTF8_count = _UTF8_count_array;
	}
}

int
_UTF8_init(_RuneLocale *rl)
{
	_UTF8EncodingInfo 	*info;
	int ret;

	rl->ops = &_utf8_runeops;
/*
	rl->ops->ro_sgetrune = _UTF8_sgetrune;
	rl->ops->ro_sputrune = _UTF8_sputrune;
	rl->ops->ro_sgetmbrune = _UTF8_sgetmbrune;
	rl->ops->ro_sputmbrune = _UTF8_sputmbrune;
	rl->ops->ro_sgetcsrune = _UTF8_sgetcsrune;
	rl->ops->ro_sputcsrune = _UTF8_sputcsrune;
*/
	ret = _citrus_ctype_init(&rl);
	if (ret != 0) {
		return (ret);
	}
	ret = _citrus_stdenc_init(info);
	if (ret != 0) {
		return (ret);
	}
	_UTF8_init_count();

	_CurrentRuneLocale = rl;

	return (0);
}

static __inline void
_UTF8_init_state(_UTF8EncodingInfo * __restrict  ei, _UTF8State * __restrict psenc)
{
	psenc->chlen = 0;
}

int
_UTF8_sgetmbrune(_UTF8EncodingInfo *ei, wchar_t *pwc, const char **s, size_t n, _UTF8State *psenc, size_t *nresult)
{
	wchar_t wchar;
	const char *s0;
	int c;
	int i;
	int chlenbak;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(psenc != NULL);

	s0 = *s;

	if (s0 == NULL) {
		_UTF8_init_state(ei, psenc);
		*nresult = 0; /* state independent */
		return 0;
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
	case 3:
	case 4:
	case 5:
		break;
	default:
		/* illegal state */
		goto ilseq;
	}

	c = _UTF8_count[psenc->ch[0] & 0xff];
	if (c == 0) {
		goto ilseq;
	}
	while (psenc->chlen < c) {
		if (n < 1) {
			goto restart;
		}
		psenc->ch[psenc->chlen] = *s0++;
		psenc->chlen++;
		n--;
	}

	switch (c) {
	case 1:
		wchar = psenc->ch[0] & 0xff;
		break;
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
		wchar = psenc->ch[0] & (0x7f >> c);
		for (i = 1; i < c; i++) {
			if ((psenc->ch[i] & 0xc0) != 0x80) {
				goto ilseq;
			}
			wchar <<= 6;
			wchar |= (psenc->ch[i] & 0x3f);
		}

		_DIAGASSERT(findlen(wchar) == c);

		break;
	}

	*s = s0;

	psenc->chlen = 0;

	if (pwc) {
		*pwc = wchar;
	}

	if (!wchar) {
		*nresult = 0;
	} else {
		*nresult = c - chlenbak;
	}

	return (0);

ilseq:
	psenc->chlen = 0;
	*nresult = (size_t)-1;
	return (EILSEQ);

restart:
	*s = s0;
	*nresult = (size_t)-2;
	return (0);
}

int
_UTF8_sputmbrune(_UTF8EncodingInfo  *ei, char *s, size_t n, wchar_t wc, _UTF8State *psenc, size_t *nresult)
{
	int cnt, i, ret;
	wchar_t c;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	cnt = _UTF8_findlen(wc);
	if (cnt <= 0 || cnt > 6) {
		/* invalid UCS4 value */
		ret = EILSEQ;
		goto err;
	}
	if (n < cnt) {
		/* bound check failure */
		ret = E2BIG;
		goto err;
	}

	c = wc;
	if (s) {
		for (i = cnt - 1; i > 0; i--) {
			s[i] = 0x80 | (c & 0x3f);
			c >>= 6;
		}
		s[0] = c;
		if (cnt == 1) {
			s[0] &= 0x7f;
		} else {
			s[0] &= (0x7f >> cnt);
			s[0] |= ((0xff00 >> cnt) & 0xff);
		}
	}

	*nresult = (size_t) cnt;
	return 0;

err:
	*nresult = (size_t) - 1;
	return ret;
}

rune_t
_UTF8_sgetrune(const char *string, size_t n, const char **result)
{
	return (emulated_sgetrune(string, n, result));
}

int
_UTF8_sputrune(rune_t c, char *string, size_t n, char **result)
{
	return (emulated_sputrune(c, string, n, result));
}

int
_UTF8_sgetcsrune(_UTF8EncodingInfo * __restrict ei, wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{
	_DIAGASSERT(wc != NULL);

	if (csid != 0) {
		return (EILSEQ);
	}

	*wc = (wchar_t)idx;

	return (0);
}

int
_UTF8_sputcsrune(_UTF8EncodingInfo * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{

	_DIAGASSERT(csid != NULL && idx != NULL);

	*csid = 0;
	*idx = (_citrus_index_t)wc;

	return (0);
}
