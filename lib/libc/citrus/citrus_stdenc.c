/*	$NetBSD: citrus_stdenc_template.h,v 1.4 2008/02/09 14:56:20 junyoung Exp $	*/

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

/* Work In Progress */

/*
 * CAUTION: THIS IS NOT STANDALONE FILE
 *
 * function templates of iconv standard encoding handler for each encodings.
 *
 */

#include <sys/cdefs.h>
#include <sys/null.h>
#include <errno.h>

#include "citrus_ctype.h"
#include "citrus_ctype_template.h"
#include "citrus_stdenc.h"

/* ----------------------------------------------------------------------
 * templates for public functions
 */

int
_citrus_stdenc_init(_ENCODING_INFO * __restrict info)
{
	_ENCODING_INFO *ei;
	_ENCODING_TRAITS *et;
	int ret;

	if (sizeof(_ENCODING_INFO) > 0) {
		ei = calloc(1, sizeof(_ENCODING_INFO));
		if (ei == NULL) {
			free((void*) ei);
			return (errno);
		}
	}

	_citrus_ctype_encoding_init(ei);

	et = malloc(sizeof(*et));
	if (et == NULL) {
		free((void*) et);
		return (errno);
	}
	et->state_size = sizeof(_ENCODING_STATE);
	et->mb_cur_max = _ENCODING_MB_CUR_MAX(ei);
	ei->traits = et;
	info = ei;
	return (0);
}

void
_citrus_stdenc_uninit(_ENCODING_INFO * __restrict ei)
{
	if (ei) {
		_citrus_ctype_encoding_uninit(ei);
		free(ei);
	}
}

int
_citrus_stdenc_init_state(_ENCODING_INFO * __restrict ei, _ENCODING_STATE * __restrict ps)
{
	_citrus_ctype_init_state(ei, ps);
	return (0);
}

int
_citrus_stdenc_cstowc(_ENCODING_INFO * __restrict ei,  wchar_t * __restrict wc, _csid_t csid, _index_t idx)
{
	return (sgetcsrune(ei, wc, csid, idx));
}

int
_citrus_stdenc_wctocs(_ENCODING_INFO * __restrict ei, _csid_t * __restrict csid, _index_t * __restrict idx, wchar_t wc)
{
	return (sputcsrune(ei, csid, idx, wc));
}

int
_citrus_stdenc_mbtocs(_ENCODING_INFO * __restrict ei, _citrus_csid_t * __restrict csid, _citrus_index_t * __restrict idx, const char ** __restrict s, size_t n, _ENCODING_STATE * __restrict ps, size_t * __restrict nresult)
{
	int ret;
	wchar_t wc;

	_DIAGASSERT(nresult != NULL);

	ret = _citrus_ctype_mbrtowc_priv(ei, &wc, s, n, ps, nresult);
	if (!ret && *nresult != (size_t)-2) {
		_citrus_stdenc_wctocs(ei, csid, idx, wc);
	}
	return (ret);
}

int
_citrus_stdenc_cstomb(_ENCODING_INFO * __restrict ei, char * __restrict s, size_t n, _citrus_csid_t csid, _citrus_index_t idx, _ENCODING_STATE * __restrict ps, size_t * __restrict nresult)
{
	int ret;
	wchar_t wc;

	_DIAGASSERT(nresult != NULL);

	wc = 0;

	if (csid != _CITRUS_CSID_INVALID) {
		ret = _citrus_stdenc_cstowc(ei, &wc, csid, idx);
		if (ret) {
			return (ret);
		}
	}
	return (_citrus_ctype_wcrtomb_priv(ei, s, n, wc, ps, nresult));
}

int
_citrus_stdenc_mbtowc(_ENCODING_INFO * __restrict ei, _citrus_wc_t * __restrict wc, const char ** __restrict s, size_t n, _ENCODING_STATE * __restrict ps, size_t * __restrict nresult)
{
	return (_citrus_ctype_mbrtowc_priv(ei, wc, s, n, ps, nresult));
}

int
_citrus_stdenc_wctomb(_ENCODING_INFO * __restrict ei, char * __restrict s, size_t n, _citrus_wc_t wc, _ENCODING_STATE * __restrict ps, size_t * __restrict nresult)
{
	return (_citrus_ctype_wcrtomb_priv(ei, s, n, wc, ps, nresult));
}

size_t
_citrus_stdenc_get_state_size(_ENCODING_INFO *ei)
{
	_DIAGASSERT(ei && ei->traits);
	return (ei->traits->state_size);
}

size_t
_citrus_stdenc_get_mb_cur_max(_ENCODING_INFO *ei)
{
	_DIAGASSERT(ei && ei->traits);
	return (ei->traits->mb_cur_max);
}
