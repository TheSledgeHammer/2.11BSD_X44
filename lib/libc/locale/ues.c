/* $NetBSD: citrus_ues.c,v 1.5 2022/04/19 20:32:14 rillig Exp $ */

/*-
 * Copyright (c)2006 Citrus Project,
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
__RCSID("$NetBSD: citrus_ues.c,v 1.5 2022/04/19 20:32:14 rillig Exp $");
#endif /* LIBC_SCCS and not lint */

#include <sys/types.h>
//#include <sys/endian.h>

#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <rune.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <limits.h>

#include <citrus/citrus_types.h>
#include <citrus/citrus_ctype.h>
#include <citrus/citrus_stdenc.h>

#include "setlocale.h"

typedef _Encoding_Info				_UESEncodingInfo;
typedef _Encoding_TypeInfo 			_UESCTypeInfo;
typedef _Encoding_State				_UESState;

#define _FUNCNAME(m)				_UES_##m

rune_t	_UES_sgetrune(const char *, size_t, char const **);
int		_UES_sputrune(rune_t, char *, size_t, char **);
int		_UES_sgetmbrune(_UESEncodingInfo * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _UESState * __restrict, size_t * __restrict);
int 	_UES_sputmbrune(_UESEncodingInfo * __restrict, char * __restrict, size_t, wchar_t, _UESState * __restrict, size_t * __restrict);
int		_UES_sgetcsrune(_UESEncodingInfo * __restrict, wchar_t * __restrict, _csid_t, _index_t);
int		_UES_sputcsrune(_UESEncodingInfo * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);
int 	_UES_module_init(_UESEncodingInfo * __restrict, const void * __restrict, size_t);
void	_UES_module_uninit(_UESEncodingInfo *);

_RuneOps _ues_runeops = {
		.ro_sgetrune 	=  	_UES_sgetrune,
		.ro_sputrune 	=  	_UES_sputrune,
		.ro_sgetmbrune 	=  	_UES_sgetmbrune,
		.ro_sputmbrune 	=  	_UES_sputmbrune,
		.ro_sgetcsrune  =	_UES_sgetcsrune,
		.ro_sputcsrune	= 	_UES_sputcsrune,
		.ro_module_init = 	_UES_module_init,
		.ro_module_uninit = 	_UES_module_uninit,
};

static __inline int
to_int(int ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	else if (ch >= 'A' && ch <= 'F')
		return (ch - 'A') + 10;
	else if (ch >= 'a' && ch <= 'f')
		return (ch - 'a') + 10;
	return -1;
}

#define ESCAPE		'\\'
#define UCS2_ESC	'u'
#define UCS4_ESC	'U'

#define UCS2_BIT	16
#define UCS4_BIT	32
#define BMP_MAX		UINT32_C(0xFFFF)
#define UCS2_MAX	UINT32_C(0x10FFFF)
#define UCS4_MAX	UINT32_C(0x7FFFFFFF)

static const char *xdig = "0123456789abcdef";

static __inline int
to_str(char *s, wchar_t wc, int bit)
{
	char *p;

	p = s;
	*p++ = ESCAPE;
	switch (bit) {
	case UCS2_BIT:
		*p++ = UCS2_ESC;
		break;
	case UCS4_BIT:
		*p++ = UCS4_ESC;
		break;
	default:
		abort();
	}
	do {
		*p++ = xdig[(wc >> (bit -= 4)) & 0xF];
	} while (bit > 0);
	return p - s;
}

static __inline int
is_hi_surrogate(wchar_t wc)
{
	return wc >= 0xD800 && wc <= 0xDBFF;
}

static __inline int
is_lo_surrogate(wchar_t wc)
{
	return wc >= 0xDC00 && wc <= 0xDFFF;
}

static __inline wchar_t
surrogate_to_ucs(wchar_t hi, wchar_t lo)
{
	_DIAGASSERT(is_hi_surrogate(hi));
	_DIAGASSERT(is_lo_surrogate(lo));

	hi -= 0xD800;
	lo -= 0xDC00;
	return (hi << 10 | lo) + 0x10000;
}

static __inline void
ucs_to_surrogate(wchar_t wc, wchar_t * __restrict hi, wchar_t * __restrict lo)
{
	_DIAGASSERT(hi != NULL);
	_DIAGASSERT(lo != NULL);
	_DIAGASSERT(wc >= 0x10000);

	wc -= 0x10000;
	*hi = (wc >> 10) + 0xD800;
	*lo = (wc & 0x3FF) + 0xDC00;
}

static __inline int
is_basic(wchar_t wc)
{
	return (uint32_t) wc <= 0x9F && wc != 0x24 && wc != 0x40 && wc != 0x60;
}

int
_UES_init(rl)
	_RuneLocale *rl;
{
	int ret;

	rl->ops = &_ues_runeops;
	ret = _citrus_ctype_init((void **)&rl, rl->variable, rl->variable_len);
	if (ret != 0) {
		return (ret);
	}
	ret = _citrus_stdenc_init((void **)&rl, rl->variable, rl->variable_len);
	if (ret != 0) {
		return (ret);
	}
	rl->variable_len = sizeof(_UESEncodingInfo);
	_CurrentRuneLocale = rl;

	return (0);
}

int
_UES_sgetmbrune(_UESEncodingInfo * __restrict ei, wchar_t * __restrict pwc, const char ** __restrict s, size_t n, _UESState * __restrict psenc, size_t * __restrict nresult)
{
	const char *s0;
	int ch, head, tail, num;
	wchar_t hi, wc;

	_DIAGASSERT(ei != NULL);
	/* pwc may be null */
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(nresult != NULL);

	if (*s == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0;
		return 0;
	}
	s0 = *s;

	hi = (wchar_t) 0;
	tail = 0;

surrogate:
	wc = (wchar_t) 0;
	head = tail;
	if (psenc->chlen == head) {
		if (n-- < 1)
			goto restart;
		psenc->ch[psenc->chlen++] = *s0++;
	}
	ch = (unsigned char) psenc->ch[head++];
	if (ch == ESCAPE) {
		if (psenc->chlen == head) {
			if (n-- < 1)
				goto restart;
			psenc->ch[psenc->chlen++] = *s0++;
		}
		switch (psenc->ch[head]) {
		case UCS2_ESC:
			tail += 6;
			break;
		case UCS4_ESC:
			if (ei->mode & _MODE_C99) {
				tail = 10;
				break;
			}
			/*FALLTHROUGH*/
		default:
			tail = 0;
		}
		++head;
	}
	for (; head < tail; ++head) {
		if (psenc->chlen == head) {
			if (n-- < 1) {
restart:
				*s = s0;
				*nresult = (size_t) - 2;
				return 0;
			}
			psenc->ch[psenc->chlen++] = *s0++;
		}
		num = to_int((int) (unsigned char) psenc->ch[head]);
		if (num < 0) {
			tail = 0;
			break;
		}
		wc = (wc << 4) | num;
	}
	head = 0;
	switch (tail) {
	case 0:
		break;
	case 6:
		if (hi != (wchar_t) 0)
			break;
		if ((ei->mode & _MODE_C99) == 0) {
			if (is_hi_surrogate(wc) != 0) {
				hi = wc;
				goto surrogate;
			}
			if ((uint32_t) wc <= 0x7F /* XXX */|| is_lo_surrogate(wc) != 0)
				break;
			goto done;
		}
		/*FALLTHROUGH*/
	case 10:
		if (is_basic(wc) == 0 && (uint32_t) wc <= UCS4_MAX
				&& is_hi_surrogate(wc) == 0 && is_lo_surrogate(wc) == 0)
			goto done;
		*nresult = (size_t) - 1;
		return EILSEQ;
	case 12:
		if (is_lo_surrogate(wc) == 0)
			break;
		wc = surrogate_to_ucs(hi, wc);
		goto done;
	}
	ch = (unsigned char) psenc->ch[0];
	head = psenc->chlen;
	if (--head > 0)
		memmove(&psenc->ch[0], &psenc->ch[1], head);
	wc = (wchar_t) ch;

done:
	psenc->chlen = head;
	if (pwc != NULL)
		*pwc = wc;
	*nresult = (size_t)((wc == 0) ? 0 : (s0 - *s));
	*s = s0;

	return 0;
}

int
_UES_sputmbrune(_UESEncodingInfo * __restrict ei, char * __restrict s, size_t n, wchar_t wc, _UESState * __restrict psenc, size_t * __restrict nresult)
{
	wchar_t hi, lo;

	if (psenc->chlen != 0)
		return EINVAL;

	if ((ei->mode & _MODE_C99) ? is_basic(wc) : (uint32_t) wc <= 0x7F) {
		if (n-- < 1)
			goto e2big;
		psenc->ch[psenc->chlen++] = (char) wc;
	} else if ((uint32_t) wc <= BMP_MAX) {
		if (n < 6)
			goto e2big;
		psenc->chlen = to_str((char *)&psenc->ch[0], wc, UCS2_BIT);
	} else if ((ei->mode & _MODE_C99) == 0 && (uint32_t) wc <= UCS2_MAX) {
		if (n < 12)
			goto e2big;
		ucs_to_surrogate(wc, &hi, &lo);
		psenc->chlen += to_str((char *)&psenc->ch[0], hi, UCS2_BIT);
		psenc->chlen += to_str((char *)&psenc->ch[6], lo, UCS2_BIT);
	} else if ((ei->mode & _MODE_C99) && (uint32_t) wc <= UCS4_MAX) {
		if (n < 10)
			goto e2big;
		psenc->chlen = to_str((char *)&psenc->ch[0], wc, UCS4_BIT);
	} else {
		*nresult = (size_t) - 1;
		return EILSEQ;
	}
	memcpy(s, psenc->ch, psenc->chlen);
	*nresult = psenc->chlen;
	psenc->chlen = 0;

	return 0;

e2big:
	*nresult = (size_t)-1;
	return E2BIG;
}

rune_t
_UES_sgetrune(const char *string, size_t n, char const **result)
{
	return (emulated_sgetrune(string, n, result));
}

int
_UES_sputrune(rune_t c, char *string, size_t n, char **result)
{
	return (emulated_sputrune(c, string, n, result));
}

int
_UES_sgetcsrune(_UESEncodingInfo * __restrict ei, wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{
	/* ei seem to be unused */
	_DIAGASSERT(wc != NULL);

	if (csid != 0) {
		return EILSEQ;
	}
	*wc = (wchar_t)idx;

	return (0);
}

int
_UES_sputcsrune(_UESEncodingInfo * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
	/* ei seem to be unused */
	_DIAGASSERT(csid != NULL);
	_DIAGASSERT(idx != NULL);

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
parse_variable(_UESEncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
	const char *p;

    p = var;
    memset((void *)ei, 0, sizeof(*ei));
	while (lenvar > 0) {
		switch (toupper(*p)) {
		case 'C':
			MATCH(C99, ei->mode |= _MODE_C99);
			break;
		}
		++p;
		--lenvar;
	}
	ei->mb_cur_max = (ei->mode & _MODE_C99) ? 10 : 12;
}

int
_UES_module_init(_UESEncodingInfo * __restrict ei, const void * __restrict var, size_t lenvar)
{
    _DIAGASSERT(ei != NULL);

    parse_variable(ei, var, lenvar);

	return (0);
}

void
_UES_module_uninit(_UESEncodingInfo *ei)
{

}
