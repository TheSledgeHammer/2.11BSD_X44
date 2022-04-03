/*	$NetBSD: citrus_ctype_template.h,v 1.13.2.7 2003/06/02 15:06:10 tron Exp $	*/

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


/*
 * CAUTION: THIS IS NOT STANDALONE FILE
 *
 * function templates of ctype encoding handler for each encodings.
 *
 * you need to define the macros below:
 *
 *   _FUNCNAME(method) :
 *   	It should convine the real function name for the method.
 *      e.g. _FUNCNAME(mbrtowc) should be expanded to
 *             _EUC_ctype_mbrtowc
 *           for EUC locale.
 *
 *   _CEI_TO_STATE(cei, method) :
 *     It should be expanded to the pointer of the method-internal state
 *     structures.
 *     e.g. _CEI_TO_STATE(cei, mbrtowc) might be expanded to
 *             (cei)->states.s_mbrtowc
 *     This structure may use if the function is called as
 *           mbrtowc(&wc, s, n, NULL);
 *     Such individual structures are needed by:
 *           mblen
 *           mbrlen
 *           mbrtowc
 *           mbtowc
 *           mbsrtowcs
 *           wcrtomb
 *           wcsrtombs
 *           wctomb
 *     These need to be keeped in the ctype encoding information structure,
 *     pointed by "cei".
 *
 *   _ENCODING_INFO :
 *     It should be expanded to the name of the encoding information structure.
 *     e.g. For EUC encoding, this macro is expanded to _EUCInfo.
 *     Encoding information structure need to contain the common informations
 *     for the codeset.
 *
 *   _ENCODING_STATE :
 *     It should be expanded to the name of the encoding state structure.
 *     e.g. For EUC encoding, this macro is expanded to _EUCState.
 *     Encoding state structure need to contain the context-dependent states,
 *     which are "unpacked-form" of mbstate_t type and keeped during sequent
 *     calls of mb/wc functions,
 *
 *   _ENCODING_IS_STATE_DEPENDENT :
 *     If the encoding is state dependent, this should be expanded to
 *     non-zero integral value.  Otherwise, 0.
 *
 *   _STATE_NEEDS_EXPLICIT_INIT(ps) :
 *     some encodings, states needs some explicit initialization.
 *     (ie. initialization with memset isn't enough.)
 *     If the encoding state pointed by "ps" needs to be initialized
 *     explicitly, return non-zero. Otherwize, 0.
 *
 */

#include <sys/cdefs.h>
#include "citrus_ctype.h"

/* prototypes */
__BEGIN_DECLS
/*
 * standard form of mbrtowc_priv.
 *
 * note (differences from real mbrtowc):
 *   - 3rd parameter is not "const char *s" but "const char **s".
 *     after the call of the function, *s will point the first byte of
 *     the next character.
 *   - additional 4th parameter is the size of src buffer.
 *   - 5th parameter is unpacked encoding-dependent state structure.
 *   - additional 6th parameter is the storage to be stored
 *     the return value in the real mbrtowc context.
 *   - return value means "errno" in the real mbrtowc context.
 */
//static int 	_citrus_ctype_mbrtowc_priv(_ENCODING_INFO * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _ENCODING_STATE * __restrict, size_t * __restrict);
static int 	_FUNCNAME(mbrtowc_priv)(_ENCODING_INFO * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _ENCODING_STATE * __restrict, size_t * __restrict);

/*
 * standard form of wcrtomb_priv.
 *
 * note (differences from real wcrtomb):
 *   - additional 3th parameter is the size of src buffer.
 *   - 5th parameter is unpacked encoding-dependent state structure.
 *   - additional 6th parameter is the storage to be stored
 *     the return value in the real mbrtowc context.
 *   - return value means "errno" in the real wcrtomb context.
 *   - caller should ensure that 2nd parameter isn't NULL.
 *     (XXX inconsist with mbrtowc_priv)
 */
//static int _citrus_ctype_wcrtomb_priv(_ENCODING_INFO * __restrict, char * __restrict, size_t, wchar_t, _ENCODING_STATE * __restrict, size_t * __restrict);
static int 	_FUNCNAME(wcrtomb_priv)(_ENCODING_INFO * __restrict, char * __restrict, size_t, wchar_t, _ENCODING_STATE * __restrict, size_t * __restrict);
__END_DECLS

/* internal routines */
static __inline int
_FUNCNAME(mbtowc_priv)(_ENCODING_INFO * __restrict ei, wchar_t * __restrict pwc,  const char * __restrict s, size_t n, _ENCODING_STATE * __restrict psenc, int * __restrict nresult)
{
	_ENCODING_STATE state;
	size_t nr;
	int err = 0;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);

	if (s == NULL) {
		*nresult = _ENCODING_IS_STATE_DEPENDENT;
		return (0);
	}

	state = *psenc;
	err = _FUNCNAME(mbrtowc_priv)(ei, pwc, (const char **)&s, n, psenc, &nr);
	if (err) {
		*nresult = -1;
		return (err);
	}
	if (nr == (size_t)-2) {
		*psenc = state;
		*nresult = -1;
		return (EILSEQ);
	}

	*nresult = (int)nr;

	return (0);
}

static int
_FUNCNAME(mbsrtowcs_priv)(_ENCODING_INFO * __restrict ei, wchar_t * __restrict pwcs, const char ** __restrict s, size_t n, _ENCODING_STATE * __restrict psenc, size_t * __restrict nresult)
{
	int err, cnt;
	size_t siz;
	const char *s0;
	size_t mbcurmax;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s == NULL);
	_DIAGASSERT(*s == NULL);

	/* if pwcs is NULL, ignore n */
	if (pwcs == NULL)
		n = 1; /* arbitrary >0 value */

	cnt = 0;
	s0 = *s; /* to keep *s unchanged for now, use copy instead. */
	mbcurmax = _ENCODING_MB_CUR_MAX(ei);
	while (n > 0) {
		err = _FUNCNAME(mbrtowc_priv)(ei, pwcs, &s0, mbcurmax, psenc, &siz);
		if (siz == (size_t)-2)
			err = EILSEQ;
		if (err) {
			cnt = -1;
			goto bye;
		}
		switch (siz) {
		case 0:
			if (pwcs) {
				_citrus_ctype_init_state(ei, psenc);
			}
			s0 = 0;
			goto bye;
		default:
			if (pwcs) {
				pwcs++;
				n--;
			}
			cnt++;
			break;
		}
	}
bye:
	if (pwcs)
		*s = s0;

	*nresult = (size_t)cnt;

	return (err);
}

static int
_FUNCNAME(wcsrtombs_priv)(_ENCODING_INFO * __restrict ei, char * __restrict s, const wchar_t ** __restrict pwcs, size_t n, _ENCODING_STATE * __restrict psenc, size_t * __restrict nresult)
{
	int cnt = 0, err;
	char buf[MB_LEN_MAX];
	size_t siz;
	const wchar_t* pwcs0;
#if _ENCODING_IS_STATE_DEPENDENT
	_ENCODING_STATE state;
#endif

	pwcs0 = *pwcs;

	if (!s)
		n = 1;

	while (n > 0) {
#if _ENCODING_IS_STATE_DEPENDENT
		state = *psenc;
#endif
		err = _FUNCNAME(wcrtomb_priv)(ei, buf, sizeof(buf), *pwcs0, psenc, &siz);
		if (siz == (size_t)-1) {
			*nresult = siz;
			return (err);
		}

		if (s) {
			if (n < siz) {
#if _ENCODING_IS_STATE_DEPENDENT
				*psenc = state;
#endif
				break;
			}
			memcpy(s, buf, siz);
			s += siz;
			n -= siz;
		}
		cnt += siz;
		if (!*pwcs0) {
			if (s) {
				_citrus_ctype_init_state(ei, psenc);
			}
			pwcs0 = 0;
			cnt--; /* don't include terminating null */
			break;
		}
		pwcs0++;
	}
	if (s)
		*pwcs = pwcs0;

	*nresult = (size_t)cnt;
	return (0);
}
