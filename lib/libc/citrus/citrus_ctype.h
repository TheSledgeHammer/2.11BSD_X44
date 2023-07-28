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


typedef int (*module_init_t)(_ENCODING_INFO * __restrict , const void * __restrict, size_t);

//int _citrus_init(void ** __restrict, const void * __restrict, size_t, module_init_t);
*/

/* prototypes */
__BEGIN_DECLS

int		_citrus_stdenc_open(void ** __restrict, void * __restrict, size_t, module_init_t);

#ifdef notyet
void		_citrus_ctype_init_state(_ENCODING_INFO * __restrict, _ENCODING_STATE * __restrict);
void		_citrus_ctype_pack_state(_ENCODING_INFO * __restrict, void * __restrict, const _ENCODING_STATE * __restrict);
void		_citrus_ctype_unpack_state(_ENCODING_INFO * __restrict, _ENCODING_STATE * __restrict, const void * __restrict);
unsigned 	_citrus_ctype_get_mb_cur_max(void *);
void		_citrus_ctype_encoding_init(_ENCODING_INFO * __restrict);
void		_citrus_ctype_encoding_uninit(_ENCODING_INFO * __restrict);
int			_citrus_ctype_init(void ** __restrict/*, size_t*/);
void		_citrus_ctype_uninit(void *);
int			_citrus_ctype_put_state_reset(void * __restrict, char * __restrict, size_t, void * __restrict, size_t * __restrict);
int 		_citrus_ctype_mblen(void * __restrict, const char * __restrict, size_t, int * __restrict);
int 		_citrus_ctype_mbrlen(void * __restrict, const char * __restrict, size_t, void * __restrict, size_t * __restrict);
int 		_citrus_ctype_mbrtowc(void * __restrict, wchar_t * __restrict, const char * __restrict, size_t, void * __restrict, size_t * __restrict);
int 		_citrus_ctype_mbsinit(void * __restrict, const void * __restrict, int * __restrict);
int 		_citrus_ctype_mbsrtowcs(void * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, void * __restrict, size_t * __restrict);
int			_citrus_ctype_mbsnrtowcs(void * __restrict, wchar_t * __restrict, const char ** __restrict, size_t, size_t, void * __restrict, size_t * __restrict);
int 		_citrus_ctype_mbstowcs(void * __restrict, wchar_t * __restrict, const char * __restrict, size_t, size_t * __restrict);
int 		_citrus_ctype_mbtowc(void * __restrict, wchar_t * __restrict, const char * __restrict, size_t, int * __restrict);
int 		_citrus_ctype_wcrtomb(void * __restrict, char * __restrict, wchar_t, void * __restrict, size_t * __restrict);
int 		_citrus_ctype_wcsrtombs(void * __restrict, char * __restrict, const wchar_t ** __restrict, size_t, void * __restrict, size_t * __restrict);
int			_citrus_ctype_wcsnrtombs(void * __restrict, char * __restrict, const wchar_t ** __restrict, size_t, size_t, void * __restrict, size_t * __restrict);
int 		_citrus_ctype_wcstombs(void * __restrict, char * __restrict, const wchar_t * __restrict, size_t, size_t * __restrict);
int 		_citrus_ctype_wctomb(void * __restrict, char * __restrict, wchar_t, int * __restrict);
int 		_citrus_ctype_btowc(void * __restrict, int, wint_t * __restrict);
int 		_citrus_ctype_wctob(void * __restrict, wint_t, int * __restrict);
#endif
__END_DECLS
#endif /* _CITRUS_CTYPE_H_ */
