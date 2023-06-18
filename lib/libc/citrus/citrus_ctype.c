
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
#include <sys/null.h>
#include <errno.h>

#include "citrus_ctype.h"
#include "citrus_ctype_template.h"

/* internal routines */
int
_citrus_ctype_mbrtowc_priv(_ENCODING_INFO * __restrict ei, wchar_t * __restrict pwc, const char ** __restrict s, size_t n, _ENCODING_STATE * __restrict psenc, size_t * __restrict nresult)
{
	return (sgetmbrune(ei, pwc, s, n, psenc, nresult));
}

int
_citrus_ctype_wcrtomb_priv(_ENCODING_INFO * __restrict ei, char * __restrict s, size_t n, wchar_t pwc, _ENCODING_STATE * __restrict psenc, size_t * __restrict nresult)
{
	return (sputmbrune(ei, s, n, pwc, psenc, nresult));
}

/* ----------------------------------------------------------------------
 * templates for public functions
 */

void
_citrus_ctype_init_state(_ENCODING_INFO * __restrict ei, _ENCODING_STATE * __restrict s)
{
	memset(s, 0, sizeof(*s));
}

void
_citrus_ctype_pack_state(_ENCODING_INFO * __restrict ei, void * __restrict pspriv, const _ENCODING_STATE * __restrict s)
{
	memcpy(pspriv, (const void *)s, sizeof(*s));
}

void
_citrus_ctype_unpack_state(_ENCODING_INFO *__restrict ei, _ENCODING_STATE * __restrict s, const void *__restrict pspriv)
{
	memcpy((void *)s, pspriv, sizeof(*s));
}

#define _RESTART_BEGIN(_func_, _cei_, _pspriv_, _pse_)								\
do {																				\
	_ENCODING_STATE _state;															\
	do {																			\
		if (_pspriv_ == NULL) {														\
			_pse_ = &_CEI_TO_STATE(_cei_, _func_);									\
			if (_STATE_NEEDS_EXPLICIT_INIT(_pse_)) {								\
				_citrus_ctype_init_state(_CEI_TO_EI(_cei_), (_pse_));				\
			} else {																\
				_pse_ = &_state;													\
				_citrus_ctype_unpack_state(_CEI_TO_EI(_cei_), _pse_, _pspriv_); 	\
			}																		\
		}																			\
} while (/*CONSTCOND*/0)

#define _RESTART_END(_func_, _cei_, _pspriv_, _pse_)								\
	if (_pspriv_ != NULL) {															\
		_citrus_ctype_pack_state(_CEI_TO_EI(_cei_), _pspriv_, _pse_);				\
	}																				\
} while (/*CONSTCOND*/0)

unsigned
/*ARGSUSED*/
_citrus_ctype_get_mb_cur_max(void *cl)
{
	return _ENCODING_MB_CUR_MAX(_CEI_TO_EI(_TO_CEI(cl)));
}

void
_citrus_ctype_encoding_init(_ENCODING_INFO *ei, _ENCODING_STATE *s)
{
	_DIAGASSERT(ei != NULL);

	memset((void *)ei, 0, sizeof(_ENCODING_INFO));
	_citrus_ctype_init_state(ei, s);
}

int
_citrus_ctype_init(void ** __restrict cl, void * __restrict var, size_t lenvar, size_t lenps)
{
	_CTYPE_INFO *cei;

	_DIAGASSERT(cl != NULL);

	/* sanity check to avoid overruns */
	if (sizeof(_ENCODING_STATE) > lenps) {
		return (EINVAL);
	}
	cei = calloc(1, sizeof(_CTYPE_INFO));
	if (cei == NULL) {
		return (ENOMEM);
	}

	*cl = (void *)cei;

	//return _FUNCNAME(encoding_module_init)(_CEI_TO_EI(cei), var, lenvar);
	return (0);
}

void
_citrus_ctype_uninit(void *cl)
{
	if (cl) {
		//_FUNCNAME(encoding_module_uninit)(_CEI_TO_EI(_TO_CEI(cl)));
		free(cl);
	}
}

int
_citrus_ctype_put_state_reset(void * __restrict cl, char * __restrict s, size_t n, void * __restrict ps, size_t * __restrict nresult)
{
	*nresult = 0;
	return (0);
}

int
_citrus_ctype_mblen(void * __restrict cl, const char * __restrict s, size_t n, int * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;

	_DIAGASSERT(cl != NULL);

	psenc = &_CEI_TO_STATE(_TO_CEI(cl), mblen);
	ei = _CEI_TO_EI(_TO_CEI(cl));
	if (_STATE_NEEDS_EXPLICIT_INIT(psenc))
		_citrus_ctype_init_state(ei, psenc);
	return _FUNCNAME(mbtowc_priv)(ei, NULL, s, n, psenc, nresult);
}

int
_citrus_ctype_mbrlen(void * __restrict cl, const char * __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(mbrlen, _TO_CEI(cl), pspriv, psenc);
	if (s == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0;
	} else {
		err = _citrus_ctype_mbrtowc_priv(ei, NULL, (const char **)&s, n,
		    (void *)psenc, nresult);
	}
	_RESTART_END(mbrlen, _TO_CEI(cl), pspriv, psenc);

	return (err);
}

int
_citrus_ctype_mbrtowc(void * __restrict cl, wchar_t * __restrict pwc, const char * __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(mbrtowc, _TO_CEI(cl), pspriv, psenc);
	if (s == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = 0;
	} else {
		err = _citrus_ctype_mbrtowc_priv(ei, pwc, (const char **)&s, n, (void *)psenc, nresult);
	}
	_RESTART_END(mbrtowc, _TO_CEI(cl), pspriv, psenc);

	return (err);
}

int
/*ARGSUSED*/
_citrus_ctype_mbsinit(void * __restrict cl, const void * __restrict pspriv, int * __restrict nresult)
{
	_ENCODING_STATE state;

	if (pspriv == NULL) {
		*nresult = 1;
		return (0);
	}

	_citrus_ctype_unpack_state(_CEI_TO_EI(_TO_CEI(cl)), &state, pspriv);

	*nresult = (state.chlen == 0); /* XXX: FIXME */

	return (0);
}

int
_citrus_ctype_mbsrtowcs(void * __restrict cl, wchar_t * __restrict pwcs, const char ** __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(mbsrtowcs, _TO_CEI(cl), pspriv, psenc);
	err = _FUNCNAME(mbsrtowcs_priv)(ei, pwcs, s, n, psenc, nresult);
	_RESTART_END(mbsrtowcs, _TO_CEI(cl), pspriv, psenc);

	return (err);
}

int
_citrus_ctype_mbstowcs(void * __restrict cl, wchar_t * __restrict pwcs, const char * __restrict s, size_t n, size_t * __restrict nresult)
{
	int err;
	_ENCODING_STATE state;
	_ENCODING_INFO *ei;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_citrus_ctype_init_state(ei, &state);
	err = _FUNCNAME(mbsrtowcs_priv)(ei, pwcs, (const char **)&s, n, &state, nresult);
	if (*nresult == (size_t)-2) {
		err = EINVAL;
		*nresult = (size_t)-1;
	}

	return (err);
}

int
_citrus_ctype_mbtowc(void * __restrict cl, wchar_t * __restrict pwc, const char * __restrict s, size_t n, int * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;

	_DIAGASSERT(cl != NULL);

	psenc = &_CEI_TO_STATE(_TO_CEI(cl), mbtowc);
	ei = _CEI_TO_EI(_TO_CEI(cl));
	if (_STATE_NEEDS_EXPLICIT_INIT(psenc))
		_citrus_ctype_init_state(ei, psenc);
	return _FUNCNAME(mbtowc_priv)(ei, pwc, s, n, psenc, nresult);
}

int
_citrus_ctype_wcrtomb(void * __restrict cl, char * __restrict s, wchar_t wc, void * __restrict pspriv, size_t * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	char buf[MB_LEN_MAX];
	int err = 0;
	size_t sz;
#if _ENCODING_IS_STATE_DEPENDENT
	size_t rsz = 0;
#endif

	_DIAGASSERT(cl != NULL);

	if (s == NULL) {
		/*
		 * use internal buffer.
		 */
		s = buf;
		wc = L'\0'; /* SUSv3 */
	}

	_RESTART_BEGIN(wcrtomb, _TO_CEI(cl), pspriv, psenc);
	sz = _ENCODING_MB_CUR_MAX(_CEI_TO_EI(_TO_CEI(cl)));
#if _ENCODING_IS_STATE_DEPENDENT
	if (wc == L'\0') {
		/* reset state */

		err = _citrus_ctype_put_state_reset(_CEI_TO_EI(_TO_CEI(cl)), s, sz, psenc, &rsz);
		if (err) {
			*nresult = -1;
			goto quit;
		}
		s += rsz;
		sz -= rsz;
	}
#endif
	err = _citrus_ctype_wcrtomb_priv(_CEI_TO_EI(_TO_CEI(cl)), s, sz, wc, psenc, nresult);
#if _ENCODING_IS_STATE_DEPENDENT
	if (err == 0)
		*nresult += rsz;
quit:
#endif
	if (err == E2BIG)
		err = EINVAL;
	_RESTART_END(wcrtomb, _TO_CEI(cl), pspriv, psenc);

	return err;
}

int
/*ARGSUSED*/
_citrus_ctype_wcsrtombs(void * __restrict cl, char * __restrict s, const wchar_t ** __restrict pwcs, size_t n, void * __restrict pspriv, size_t * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(wcsrtombs, _TO_CEI(cl), pspriv, psenc);
	err = _FUNCNAME(wcsrtombs_priv)(ei, s, pwcs, n, psenc, nresult);
	_RESTART_END(wcsrtombs, _TO_CEI(cl), pspriv, psenc);

	return err;
}

int
/*ARGSUSED*/
_citrus_ctype_wcstombs(void * __restrict cl, char * __restrict s, const wchar_t * __restrict pwcs, size_t n, size_t * __restrict nresult)
{
	_ENCODING_STATE state;
	_ENCODING_INFO *ei;
	int err;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_citrus_ctype_init_state(ei, &state);
	err = _FUNCNAME(wcsrtombs_priv)(ei, s, (const wchar_t **)&pwcs, n, &state, nresult);

	return err;
}

int
_citrus_ctype_wctomb(void * __restrict cl, char * __restrict s, wchar_t wc, int * __restrict nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	size_t nr, sz;
#if _ENCODING_IS_STATE_DEPENDENT
	size_t rsz = 0;
#endif
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	psenc = &_CEI_TO_STATE(_TO_CEI(cl), wctomb);
	if (_STATE_NEEDS_EXPLICIT_INIT(psenc))
		_citrus_ctype_init_state(ei, psenc);
	if (s == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = _ENCODING_IS_STATE_DEPENDENT;
		return 0;
	}
	sz = _ENCODING_MB_CUR_MAX(_CEI_TO_EI(_TO_CEI(cl)));
#if _ENCODING_IS_STATE_DEPENDENT
	if (wc == L'\0') {
		/* reset state */

		err = _citrus_ctype_put_state_reset(_CEI_TO_EI(_TO_CEI(cl)), s, sz, psenc, &rsz);
		if (err) {
			*nresult = -1;
			return 0;
		}
		s += rsz;
		sz -= rsz;
	}
#endif
	err = _citrus_ctype_wcrtomb_priv(ei, s, sz, wc, psenc, &nr);
#if _ENCODING_IS_STATE_DEPENDENT
	if (err == 0)
		*nresult = (int)(nr + rsz);
	else
#endif
	*nresult = (int)nr;

	return 0;
}

int
/*ARGSUSED*/
_citrus_ctype_btowc(void * __restrict cl, int c, wint_t * __restrict wcresult)
{
	_ENCODING_STATE state;
	_ENCODING_INFO *ei;
	char mb;
	char const *s;
	wchar_t wc;
	size_t nr;
	int err;

	_DIAGASSERT(cl != NULL);

	if (c == EOF) {
		*wcresult = WEOF;
		return 0;
	}
	ei = _CEI_TO_EI(_TO_CEI(cl));
	_citrus_ctype_init_state(ei, &state);
	mb = (char)(unsigned)c;
	s = &mb;
	err = _citrus_ctype_mbrtowc_priv(ei, &wc, &s, 1, &state, &nr);
	if (!err && (nr == 0 || nr == 1))
		*wcresult = (wint_t)wc;
	else
		*wcresult = WEOF;

	return 0;
}

int
/*ARGSUSED*/
_citrus_ctype_wctob(void * __restrict cl, wint_t wc, int * __restrict cresult)
{
	_ENCODING_STATE state;
	_ENCODING_INFO *ei;
	char buf[MB_LEN_MAX];
	size_t nr;
	int err;

	_DIAGASSERT(cl != NULL);

	if (wc == WEOF) {
		*cresult = EOF;
		return 0;
	}
	ei = _CEI_TO_EI(_TO_CEI(cl));
	_citrus_ctype_init_state(ei, &state);
	err = _citrus_ctype_wcrtomb_priv(ei, buf, _ENCODING_MB_CUR_MAX(ei), (wchar_t)wc, &state, &nr);
	if (!err && nr == 1)
		*cresult = buf[0];
	else
		*cresult = EOF;

	return 0;
}
