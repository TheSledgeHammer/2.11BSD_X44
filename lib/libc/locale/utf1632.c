/*	$NetBSD: citrus_utf1632.c,v 1.3 2003/06/27 12:55:13 yamt Exp $	*/

/*-
 * Copyright (c)2003 Citrus Project,
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: citrus_utf1632.c,v 1.3 2003/06/27 12:55:13 yamt Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
#include <sys/endian.h>

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

typedef _Encoding_Info				_UTF1632EncodingInfo;
typedef _Encoding_TypeInfo 			_UTF1632CTypeInfo;
typedef _Encoding_State				_UTF1632State;

#define _FUNCNAME(m)				_UTF1632_##m

rune_t	_UTF1632_sgetrune(const char *, size_t, char const **);
int		_UTF1632_sputrune(rune_t, char *, size_t, char **);
int		_UTF1632_sgetmbrune(_UTF1632EncodingInfo * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _UTF1632State * __restrict, size_t * __restrict);
int 	_UTF1632_sputmbrune(_UTF1632EncodingInfo * __restrict , char * __restrict, size_t, wchar_t, _UTF1632State * __restrict, size_t * __restrict);
int		_UTF1632_sgetcsrune(_UTF1632EncodingInfo * __restrict, wchar_t * __restrict, _csid_t, _index_t);
int		_UTF1632_sputcsrune(_UTF1632EncodingInfo * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);
int 	_UTF1632_module_init(_UTF1632EncodingInfo * __restrict, const void * __restrict, size_t);
void	_UTF1632_module_uninit(_UTF1632EncodingInfo *);

_RuneOps _utf1632_runeops = {
		.ro_sgetrune 	=  	_UTF1632_sgetrune,
		.ro_sputrune 	=  	_UTF1632_sputrune,
		.ro_sgetmbrune 	=  	_UTF1632_sgetmbrune,
		.ro_sputmbrune 	=  	_UTF1632_sputmbrune,
		.ro_sgetcsrune  =	_UTF1632_sgetcsrune,
		.ro_sputcsrune	= 	_UTF1632_sputcsrune,
		.ro_module_init = 	_UTF1632_module_init,
		.ro_module_uninit = 	_UTF1632_module_uninit,
};

int
_UTF1632_init(rl)
	_RuneLocale *rl;
{
	int ret;

	rl->ops = &_utf1632_runeops;
	ret = _citrus_ctype_init((void **)&rl, rl->variable, rl->variable_len);
	if (ret != 0) {
		return (ret);
	}
	ret = _citrus_stdenc_init((void **)&rl, rl->variable, rl->variable_len);
	if (ret != 0) {
		return (ret);
	}
	rl->variable_len = sizeof(_UTF1632EncodingInfo);
	_CurrentRuneLocale = rl;

	return (0);
}

int
_UTF1632_sgetmbrune(_UTF1632EncodingInfo * __restrict ei, wchar_t * __restrict pwc, const char ** __restrict s, size_t n, _UTF1632State * __restrict psenc, size_t * __restrict nresult)
{
	int chlenbak, endian, needlen;
	wchar_t wc = 0;
	size_t result;
	const char *s0;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(psenc != NULL);

	s0 = *s;

	if (s0 == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0; /* state independent */
		return (0);
	}

	result = 0;
	chlenbak = psenc->chlen;

refetch:
	if ((ei->mode & _MODE_UTF32) != 0 || chlenbak>=2)
		needlen = 4;
	else
		needlen = 2;

	while (chlenbak < needlen) {
		if (n==0)
			goto restart;
		psenc->ch[chlenbak++] = *s0++;
		n--;
		result++;
	}

    if (psenc->current_endian == _ENDIAN_UNKNOWN) {
        if ((ei->mode & _MODE_FORCE_ENDIAN) == 0) {
			/* judge endian marker */
			if ((ei->mode & _MODE_UTF32) == 0) {
				/* UTF16 */
				if (psenc->ch[0]==0xFE && psenc->ch[1]==0xFF) {
					psenc->current_endian = _ENDIAN_BIG;
					chlenbak = 0;
					goto refetch;
				} else if (psenc->ch[0]==0xFF && psenc->ch[1]==0xFE) {
					psenc->current_endian = _ENDIAN_LITTLE;
					chlenbak = 0;
					goto refetch;
				}
			} else {
				/* UTF32 */
				if (psenc->ch[0]==0x00 && psenc->ch[1]==0x00 &&
				    psenc->ch[2]==0xFE && psenc->ch[3]==0xFF) {
					psenc->current_endian = _ENDIAN_BIG;
					chlenbak = 0;
					goto refetch;
				} else if (psenc->ch[0]==0xFF && psenc->ch[1]==0xFE &&
					   psenc->ch[2]==0x00 && psenc->ch[3]==0x00) {
					psenc->current_endian = _ENDIAN_LITTLE;
					chlenbak = 0;
					goto refetch;
				}
			}
		}
		psenc->current_endian = ei->preffered_endian;
	}
	endian = psenc->current_endian;

	/* get wc */
	if ((ei->mode & _MODE_UTF32) == 0) {
		/* UTF16 */
		if (needlen==2) {
			switch (endian) {
			case _ENDIAN_LITTLE:
				wc = (psenc->ch[0] |
				      ((wchar_t)psenc->ch[1] << 8));
				break;
			case _ENDIAN_BIG:
				wc = (psenc->ch[1] |
				      ((wchar_t)psenc->ch[0] << 8));
				break;
			default:
				goto ilseq;
			}
			if (wc >= 0xD800 && wc <= 0xDBFF) {
				/* surrogate high */
				needlen=4;
				goto refetch;
			}
		} else {
			/* surrogate low */
			wc -= 0xD800; /* wc : surrogate high (see above) */
			wc <<= 10;
			switch (endian) {
			case _ENDIAN_LITTLE:
				if (psenc->ch[3]<0xDC || psenc->ch[3]>0xDF)
					goto ilseq;
				wc |= psenc->ch[2];
				wc |= (wchar_t)(psenc->ch[3] & 3) << 8;
				break;
			case _ENDIAN_BIG:
				if (psenc->ch[2]<0xDC || psenc->ch[2]>0xDF)
					goto ilseq;
				wc |= psenc->ch[3];
				wc |= (wchar_t)(psenc->ch[2] & 3) << 8;
				break;
			default:
				goto ilseq;
			}
			wc += 0x10000;
		}
	} else {
		/* UTF32 */
		switch (endian) {
		case _ENDIAN_LITTLE:
			wc = (psenc->ch[0] |
			      ((wchar_t)psenc->ch[1] << 8) |
			      ((wchar_t)psenc->ch[2] << 16) |
			      ((wchar_t)psenc->ch[3] << 24));
			break;
		case _ENDIAN_BIG:
			wc = (psenc->ch[3] |
			      ((wchar_t)psenc->ch[2] << 8) |
			      ((wchar_t)psenc->ch[1] << 16) |
			      ((wchar_t)psenc->ch[0] << 24));
			break;
		default:
			goto ilseq;
		}
		if (wc >= 0xD800 && wc <= 0xDFFF)
			goto ilseq;
	}

	*pwc = wc;
	psenc->chlen = 0;
	*nresult = result;
	*s = s0;

	return (0);

ilseq:
	*nresult = (size_t)-1;
	psenc->chlen = 0;
	return (EILSEQ);

restart:
	*nresult = (size_t)-2;
	psenc->chlen = chlenbak;
	*s = s0;
	return (0);
}

int
_UTF1632_sputmbrune(_UTF1632EncodingInfo * __restrict ei, char * __restrict s, size_t n, wchar_t wc, _UTF1632State * __restrict psenc, size_t * __restrict nresult)
{
	wchar_t wc2;
	static const char _bom[4] = {
#if BYTE_ORDER == BIG_ENDIAN
	    0x00, 0x00, 0xFE, 0xFF,
#else
	    0xFF, 0xFE, 0x00, 0x00,
#endif
	};
	const char *bom = &_bom[0];
	size_t cnt;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(s != NULL);

	cnt = (size_t)0;
	if (psenc->current_endian == _ENDIAN_UNKNOWN) {
		if ((ei->mode & _MODE_FORCE_ENDIAN) == 0) {
			if (ei->mode & _MODE_UTF32) {
				cnt = 4;
			} else {
				cnt = 2;
#if BYTE_ORDER == BIG_ENDIAN
				bom += 2;
#endif
			}
			if (n < cnt)
				goto e2big;
			memcpy(s, bom, cnt);
			s += cnt, n -= cnt;
		}
		psenc->current_endian = ei->preffered_endian;
	}

	wc2 = 0;
	if ((ei->mode & _MODE_UTF32)==0) {
		/* UTF16 */
		if (wc>0xFFFF) {
			/* surrogate */
			if (wc>0x10FFFF)
				goto ilseq;
			if (n < 4)
				goto e2big;
			cnt += 4;
			wc -= 0x10000;
			wc2 = (wc & 0x3FF) | 0xDC00;
			wc = (wc>>10) | 0xD800;
		} else {
			if (n < 2)
				goto e2big;
			cnt += 2;
		}

surrogate:
		switch (psenc->current_endian) {
		case _ENDIAN_BIG:
			s[1] = wc;
			s[0] = (wc >>= 8);
			break;
		case _ENDIAN_LITTLE:
			s[0] = wc;
			s[1] = (wc >>= 8);
			break;
		}
		if (wc2!=0) {
			wc = wc2;
			wc2 = 0;
			s += 2;
			goto surrogate;
		}
	} else {
		/* UTF32 */
		if (wc >= 0xD800 && wc <= 0xDFFF)
			goto ilseq;
		if (n < 4)
			goto e2big;
		cnt += 4;
		switch (psenc->current_endian) {
		case _ENDIAN_BIG:
			s[3] = wc;
			s[2] = (wc >>= 8);
			s[1] = (wc >>= 8);
			s[0] = (wc >>= 8);
			break;
		case _ENDIAN_LITTLE:
			s[0] = wc;
			s[1] = (wc >>= 8);
			s[2] = (wc >>= 8);
			s[3] = (wc >>= 8);
			break;
		}
	}
	*nresult = cnt;

	return (0);

ilseq:
	*nresult = (size_t)-1;
	return (EILSEQ);
e2big:
	*nresult = (size_t)-1;
	return (E2BIG);
}

rune_t
_UTF1632_sgetrune(const char *string, size_t n, char const **result)
{
	return (emulated_sgetrune(string, n, result));
}

int
_UTF1632_sputrune(rune_t c, char *string, size_t n, char **result)
{
	return (emulated_sputrune(c, string, n, result));
}

int
_UTF1632_sgetcsrune(_UTF1632EncodingInfo * __restrict ei, wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{
	_DIAGASSERT(wc != NULL);

	if (csid != 0) {
		return (EILSEQ);
	}

	*wc = (_wc_t)idx;

	return (0);
}

int
_UTF1632_sputcsrune(_UTF1632EncodingInfo * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
	_DIAGASSERT(csid != NULL && idx != NULL);

	*csid = 0;
	*idx = (_index_t)wc;

	return (0);
}

#define MATCH(x, act) 										\
	do {													\
		if (lenvar >= (sizeof(#x)-1) &&						\
				strncasecmp(p, #x, sizeof(#x)-1) == 0) {    \
			act;											\
			lenvar -= sizeof(#x)-1;							\
			p += sizeof(#x)-1;								\
    }														\
} while (/*CONSTCOND*/0);

static void
parse_variable(_UTF1632EncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
	const char *p;

	p = var;
	while (lenvar > 0) {
		switch (*p) {
		case 'B':
		case 'b':
			MATCH(big, ei->preffered_endian = _ENDIAN_BIG);
			break;
		case 'L':
		case 'l':
			MATCH(little, ei->preffered_endian = _ENDIAN_LITTLE);
			break;
		case 'F':
		case 'f':
			MATCH(force, ei->mode |= _MODE_FORCE_ENDIAN);
			break;
		case 'U':
		case 'u':
			MATCH(utf32, ei->mode |= _MODE_UTF32);
			break;
		}
		p++;
		lenvar--;
	}
}

int
_UTF1632_module_init(_UTF1632EncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
	_DIAGASSERT(ei != NULL);

	parse_variable(ei, var, lenvar);

    if ((ei->mode & _MODE_UTF32) == 0) {
    	ei->mb_cur_max = 6; /* endian + surrogate */
    } else {
    	ei->mb_cur_max = 8; /* endian + normal */
    }

	if (ei->preffered_endian == _ENDIAN_UNKNOWN) {
#if BYTE_ORDER == BIG_ENDIAN
		ei->preffered_endian = _ENDIAN_BIG;
#else
		ei->preffered_endian = _ENDIAN_LITTLE;
#endif
	}
	return (0);
}

void
_UTF1632_module_uninit(_UTF1632EncodingInfo *ei)
{

}
