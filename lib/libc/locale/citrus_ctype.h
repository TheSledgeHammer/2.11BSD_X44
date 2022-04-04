/*	$NetBSD: citrus_ctype.h,v 1.2 2003/03/05 20:18:15 tshiozak Exp $	*/

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
 *
 */

#ifndef _CITRUS_CTYPE_H_
#define _CITRUS_CTYPE_H_

#include <encoding.h>
#include <rune.h>

__BEGIN_DECLS
int 		_citrus_ctype_mbrtowc_priv(_ENCODING_INFO * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, _ENCODING_STATE * __restrict, size_t * __restrict);
int 		_citrus_ctype_wcrtomb_priv(_ENCODING_INFO * __restrict, char * __restrict, size_t, wchar_t, _ENCODING_STATE * __restrict, size_t * __restrict);

void		_citrus_ctype_init_state(_ENCODING_INFO *ei, _ENCODING_STATE *s);
void		_citrus_ctype_pack_state(_ENCODING_INFO *ei, void *pspriv, const _ENCODING_STATE *s);
void		_citrus_ctype_unpack_state(_ENCODING_INFO *ei, _ENCODING_STATE *s, const void *pspriv);

unsigned 	_citrus_ctype_get_mb_cur_max(void *cl);
int 		_citrus_ctype_mblen(void * __restrict cl, const char * __restrict s, size_t n, int * __restrict nresult);
int 		_citrus_ctype_mbrlen(void * __restrict cl, const char * __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult);
int 		_citrus_ctype_mbrtowc(void * __restrict cl, wchar_t * __restrict pwc, const char * __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult);
int 		_citrus_ctype_mbsinit(void * __restrict cl, const void * __restrict pspriv, int * __restrict nresult);
int 		_citrus_ctype_mbsrtowcs(void * __restrict cl, wchar_t * __restrict pwcs, const char ** __restrict s, size_t n, void * __restrict pspriv, size_t * __restrict nresult);
int 		_citrus_ctype_mbstowcs(void * __restrict cl, wchar_t * __restrict pwcs, const char * __restrict s, size_t n, size_t * __restrict nresult);
int 		_citrus_ctype_mbtowc(void * __restrict cl, wchar_t * __restrict pwc, const char * __restrict s, size_t n, int * __restrict nresult);
int 		_citrus_ctype_wcrtomb(void * __restrict cl, char * __restrict s, wchar_t wc, void * __restrict pspriv, size_t * __restrict nresult);
int 		_citrus_ctype_wcsrtombs(void * __restrict cl, char * __restrict s, const wchar_t ** __restrict pwcs, size_t n, void * __restrict pspriv, size_t * __restrict nresult);
int 		_citrus_ctype_wcstombs(void * __restrict cl, char * __restrict s, const wchar_t * __restrict pwcs, size_t n, size_t * __restrict nresult);
int 		_citrus_ctype_wctomb(void * __restrict cl, char * __restrict s, wchar_t wc, int * __restrict nresult);
int 		_citrus_ctype_btowc(void * __restrict cl, int c, wint_t * __restrict wcresult);
int 		_citrus_ctype_wctob(void * __restrict cl, wint_t wc, int * __restrict cresult);
__END_DECLS
#endif /* _CITRUS_CTYPE_H_ */
