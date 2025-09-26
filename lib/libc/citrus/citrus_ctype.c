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
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "citrus_ctype.h"

/* internal routines */

static __inline int
_citrus_ctype_mbtowc_priv(_ENCODING_INFO *ei, wchar_t *pwc,  const char *s, size_t n, _ENCODING_STATE *psenc, int *nresult)
{
	_ENCODING_STATE state;
	size_t nr;
	int err = 0;

	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);

	if (s == NULL) {
		_citrus_ctype_init_state(ei, psenc);
		*nresult = _ENCODING_IS_STATE_DEPENDENT;
		return (0);
	}

	state = *psenc;
	err = _citrus_ctype_mbrtowc_priv(ei, pwc, (const char **)&s, n, psenc, &nr);
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

static __inline int
_citrus_ctype_mbsrtowcs_priv(_ENCODING_INFO *ei, wchar_t *pwcs, const char **s, size_t n, _ENCODING_STATE *psenc, size_t *nresult)
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
		err = _citrus_ctype_mbrtowc_priv(ei, pwcs, &s0, mbcurmax, psenc, &siz);
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

static __inline int
_citrus_ctype_mbsnrtowcs_priv(_ENCODING_INFO *ei, wchar_t *pwcs, const char **s, size_t in, size_t n, _ENCODING_STATE *psenc,size_t *nresult)
{
	int err;
	size_t cnt, siz;
	const char *s0, *se;

	_DIAGASSERT(nresult != 0);
	_DIAGASSERT(ei != NULL);
	_DIAGASSERT(psenc != NULL);
	_DIAGASSERT(s != NULL);
	_DIAGASSERT(*s != NULL);

	/* if pwcs is NULL, ignore n */
	if (pwcs == NULL)
		n = 1; /* arbitrary >0 value */

	err = 0;
	cnt = 0;
	se = *s + in;
	s0 = *s; /* to keep *s unchanged for now, use copy instead. */
	while (s0 < se && n > 0) {
		err = _citrus_ctype_mbrtowc_priv(ei, pwcs, &s0, se - s0, psenc, &siz);
		if (err) {
			cnt = (size_t)-1;
			goto bye;
		}
		if (siz == (size_t)-2) {
			s0 = se;
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

	*nresult = cnt;

	return err;
}

static __inline int
_citrus_ctype_wcsrtombs_priv(_ENCODING_INFO *ei, char *s, const wchar_t **pwcs, size_t n, _ENCODING_STATE *psenc, size_t *nresult)
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
		err = _citrus_ctype_wcrtomb_priv(ei, buf, sizeof(buf), *pwcs0, psenc, &siz);
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

static __inline int
_citrus_ctype_wcsnrtombs_priv(_ENCODING_INFO *ei, char *s, const wchar_t **pwcs, size_t in, size_t n, _ENCODING_STATE *psenc, size_t *nresult)
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

	while (in > 0 && n > 0) {
#if _ENCODING_IS_STATE_DEPENDENT
		state = *psenc;
#endif
		err = _citrus_ctype_wcrtomb_priv(ei, buf, sizeof(buf), *pwcs0, psenc, &siz);
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
		--in;
	}
	if (s)
		*pwcs = pwcs0;

	*nresult = (size_t)cnt;
	return (0);
}

static void
_citrus_ctype_init_charset(_ENCODING_INFO * __restrict ei, _ENCODING_STATE * __restrict s)
{
	int i;

	s->gl = 0;
	s->gr = (ei->flags & F_8BIT) ? 1 : -1;

	for (i = 0; i < 4; i++) {
		if (ei->initg[i].final) {
			s->g[i].type = ei->initg[i].type;
			s->g[i].final = ei->initg[i].final;
			s->g[i].interm = ei->initg[i].interm;
		}
	}
	s->singlegl = s->singlegr = -1;
	s->flags |= 1;
}

/* ----------------------------------------------------------------------
 * templates for public functions
 */

void
_citrus_ctype_init_state(_ENCODING_INFO * __restrict ei, _ENCODING_STATE * __restrict s)
{
	memset(s, 0, sizeof(*s));

	/* check state flags: returns if equals 0 */
	if (!_STATE_FLAG_INITIALIZED) {
		return;
	}

	/* Only ISO2022 should reach this point! */
	_citrus_ctype_init_charset(ei, s);
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
_citrus_ctype_encoding_init(_ENCODING_INFO *__restrict ei)
{
	_DIAGASSERT(ei != NULL);

	memset((void *)ei, 0, sizeof(_ENCODING_INFO));
}

void
_citrus_ctype_encoding_uninit(_ENCODING_INFO * __restrict ei)
{

}

int
_citrus_ctype_init(void ** __restrict cl, void * __restrict var, size_t lenvar /* size_t lenps */)
{
	_CTYPE_INFO  *cei;

	/* ctype_init */
	_DIAGASSERT(cl != NULL);

#ifdef notyet
	/* sanity check to avoid overruns */
	if (sizeof(_ENCODING_STATE) > lenps) {
		return (EINVAL);
	}
#endif

	cei = calloc(1, sizeof(_CTYPE_INFO));
	if (cei == NULL) {
		return (ENOMEM);
	}

	if (sizeof(_CTYPE_INFO) < lenvar) {
		memcpy(var, cei, sizeof(_CTYPE_INFO));
	} else {
		var = cei;
	}
	return (_citrus_ctype_module_init(_CEI_TO_EI(cei), var, lenvar));
}

void
_citrus_ctype_uninit(void *cl)
{
	if (cl) {
		_citrus_ctype_module_uninit(_CEI_TO_EI(_TO_CEI(cl)));
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
_citrus_ctype_mblen(void *cl, const char *s, size_t n, int *nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;

	_DIAGASSERT(cl != NULL);

	psenc = &_CEI_TO_STATE(_TO_CEI(cl), mblen);
	ei = _CEI_TO_EI(_TO_CEI(cl));
	if (_STATE_NEEDS_EXPLICIT_INIT(psenc))
		_citrus_ctype_init_state(ei, psenc);
	return _citrus_ctype_mbtowc_priv(ei, NULL, s, n, psenc, nresult);
}

int
_citrus_ctype_mbrlen(void *cl, const char *s, size_t n, void *pspriv, size_t *nresult)
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
_citrus_ctype_mbrtowc(void *cl, wchar_t *pwc, const char *s, size_t n, void *pspriv, size_t *nresult)
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
_citrus_ctype_mbsinit(void *cl, const void *pspriv, int *nresult)
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
_citrus_ctype_mbsrtowcs(void *cl, wchar_t *pwcs, const char **s, size_t n, void *pspriv, size_t *nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(mbsrtowcs, _TO_CEI(cl), pspriv, psenc);
	err = _citrus_ctype_mbsrtowcs_priv(ei, pwcs, s, n, psenc, nresult);
	_RESTART_END(mbsrtowcs, _TO_CEI(cl), pspriv, psenc);

	return (err);
}

int
_citrus_ctype_mbsnrtowcs(void *cl, wchar_t *pwcs, const char **s, size_t in, size_t n, void *pspriv, size_t *nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(mbsnrtowcs, _TO_CEI(cl), pspriv, psenc);
	err = _citrus_ctype_mbsnrtowcs_priv(ei, pwcs, s, in, n, psenc, nresult);
	_RESTART_END(mbsnrtowcs, _TO_CEI(cl), pspriv, psenc);

	return (err);
}

int
_citrus_ctype_mbstowcs(void *cl, wchar_t *pwcs, const char *s, size_t n, size_t *nresult)
{
	int err;
	_ENCODING_STATE state;
	_ENCODING_INFO *ei;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_citrus_ctype_init_state(ei, &state);
	err = _citrus_ctype_mbsrtowcs_priv(ei, pwcs, (const char **)&s, n, &state, nresult);
	if (*nresult == (size_t)-2) {
		err = EINVAL;
		*nresult = (size_t)-1;
	}

	return (err);
}

int
_citrus_ctype_mbtowc(void *cl, wchar_t *pwc, const char *s, size_t n, int *nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;

	_DIAGASSERT(cl != NULL);

	psenc = &_CEI_TO_STATE(_TO_CEI(cl), mbtowc);
	ei = _CEI_TO_EI(_TO_CEI(cl));
	if (_STATE_NEEDS_EXPLICIT_INIT(psenc))
		_citrus_ctype_init_state(ei, psenc);
	return _citrus_ctype_mbtowc_priv(ei, pwc, s, n, psenc, nresult);
}

int
_citrus_ctype_wcrtomb(void *cl, char *s, wchar_t wc, void *pspriv, size_t *nresult)
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

	return (err);
}

int
/*ARGSUSED*/
_citrus_ctype_wcsrtombs(void *cl, char *s, const wchar_t **pwcs, size_t n, void *pspriv, size_t *nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(wcsrtombs, _TO_CEI(cl), pspriv, psenc);
	err = _citrus_ctype_wcsrtombs_priv(ei, s, pwcs, n, psenc, nresult);
	_RESTART_END(wcsrtombs, _TO_CEI(cl), pspriv, psenc);

	return err;
}

int
/*ARGSUSED*/
_citrus_ctype_wcsnrtombs(void *cl, char *s, const wchar_t **pwcs, size_t in, size_t n, void *pspriv, size_t *nresult)
{
	_ENCODING_STATE *psenc;
	_ENCODING_INFO *ei;
	int err = 0;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_RESTART_BEGIN(wcsnrtombs, _TO_CEI(cl), pspriv, psenc);
	err = _citrus_ctype_wcsnrtombs_priv(ei, s, pwcs, in, n, psenc, nresult);
	_RESTART_END(wcsnrtombs, _TO_CEI(cl), pspriv, psenc);

	return err;
}

int
/*ARGSUSED*/
_citrus_ctype_wcstombs(void *cl, char *s, const wchar_t *pwcs, size_t n, size_t *nresult)
{
	_ENCODING_STATE state;
	_ENCODING_INFO *ei;
	int err;

	_DIAGASSERT(cl != NULL);

	ei = _CEI_TO_EI(_TO_CEI(cl));
	_citrus_ctype_init_state(ei, &state);
	err = _citrus_ctype_wcsrtombs_priv(ei, s, (const wchar_t **)&pwcs, n, &state, nresult);

	return err;
}

int
_citrus_ctype_wctomb(void *cl, char *s, wchar_t wc, int *nresult)
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
    if (err != 0) {
        *nresult = (int)nr;
    }
#if _ENCODING_IS_STATE_DEPENDENT
      else {
        *nresult = (int)(nr + rsz);
      }
#endif

	return 0;
}

int
/*ARGSUSED*/
_citrus_ctype_btowc(void *cl, int c, wint_t *wcresult)
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
_citrus_ctype_wctob(void *cl, wint_t wc, int *cresult)
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
