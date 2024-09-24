/*	$NetBSD: citrus_stdenc.h,v 1.4 2005/10/29 18:02:04 tshiozak Exp $	*/

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
 *
 */

#ifndef _CITRUS_STDENC_H_
#define _CITRUS_STDENC_H_

#include "citrus_rune.h"

/* get_state_desc id */
#define _CITRUS_STDENC_SDID_GENERIC				0
/* get_state_desc rstate */
#define _CITRUS_STDENC_SDGEN_UNKNOWN			0
#define _CITRUS_STDENC_SDGEN_INITIAL			1
#define _CITRUS_STDENC_SDGEN_STABLE				2
#define _CITRUS_STDENC_SDGEN_INCOMPLETE_CHAR	3
#define _CITRUS_STDENC_SDGEN_INCOMPLETE_SHIFT	4

static __inline int
_citrus_stdenc_cstowc_priv(_ENCODING_INFO *ei,  wchar_t *wc, _csid_t csid, _index_t idx)
{
	return (_citrus_rune_sgetcsrune(_CurrentRuneLocale, ei, wc, csid, idx));
}

static __inline int
_citrus_stdenc_wctocs_priv(_ENCODING_INFO *ei, _csid_t *csid, _index_t *idx, wchar_t wc)
{
	return (_citrus_rune_sputcsrune(_CurrentRuneLocale, ei, csid, idx, wc));
}

__BEGIN_DECLS
int		_citrus_stdenc_init(void ** __restrict, void * __restrict, size_t);
void	_citrus_stdenc_uninit(_ENCODING_INFO * __restrict);
int		_citrus_stdenc_init_state(_ENCODING_INFO * __restrict, _ENCODING_STATE * __restrict);
int 	_citrus_stdenc_cstowc(_ENCODING_INFO * __restrict,  wchar_t * __restrict, _csid_t, _index_t);
int 	_citrus_stdenc_wctocs(_ENCODING_INFO * __restrict, _csid_t * __restrict, _index_t * __restrict, wchar_t);
int 	_citrus_stdenc_mbtocs(_ENCODING_INFO * __restrict, _citrus_csid_t * __restrict, _citrus_index_t * __restrict, const char ** __restrict, size_t, _ENCODING_STATE * __restrict, size_t * __restrict);
int 	_citrus_stdenc_cstomb(_ENCODING_INFO * __restrict, char * __restrict, size_t, _citrus_csid_t, _citrus_index_t, _ENCODING_STATE * __restrict, size_t * __restrict);
int 	_citrus_stdenc_mbtowc(_ENCODING_INFO * __restrict, _citrus_wc_t * __restrict, const char ** __restrict, size_t, _ENCODING_STATE * __restrict, size_t * __restrict);
int 	_citrus_stdenc_wctomb(_ENCODING_INFO * __restrict, char * __restrict, size_t, _citrus_wc_t, _ENCODING_STATE * __restrict, size_t * __restrict);
size_t 	_citrus_stdenc_get_state_size(_ENCODING_INFO *);
size_t 	_citrus_stdenc_get_mb_cur_max(_ENCODING_INFO *);
int		_citrus_stdenc_put_state_reset(void * __restrict, char * __restrict, size_t, void * __restrict, size_t * __restrict);
int		_citrus_stdenc_get_state_desc(_ENCODING_INFO * __restrict, _ENCODING_STATE * __restrict, int, int * __restrict);
__END_DECLS

#endif /* _CITRUS_STDENC_H_ */
