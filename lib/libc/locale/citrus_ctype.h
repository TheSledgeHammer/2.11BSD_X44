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

#include <rune.h>

/* generic encoding structures*/
typedef struct {
	wchar_t				ch[3];
	int 				chlen;
} _Encoding_State;

typedef struct {
	unsigned			count[4];
	wchar_t				bits[4];
	wchar_t				mask;
	unsigned			mb_cur_max;
} _Encoding_Info;

typedef struct {
	_Encoding_Info 		ei;
	struct {
		_Encoding_State	s_mblen;
		_Encoding_State	s_mbrlen;
		_Encoding_State	s_mbrtowc;
		_Encoding_State	s_mbtowc;
		_Encoding_State	s_mbsrtowcs;
		_Encoding_State	s_mbsnrtowcs;
		_Encoding_State	s_wcrtomb;
		_Encoding_State	s_wcsrtombs;
		_Encoding_State	s_wcsnrtombs;
		_Encoding_State	s_wctomb;
	} states;
} _Encoding_TypeInfo;

/*
 * macros
 */
#define _CEI_TO_EI(_cei_)					(&(_cei_)->ei)
#define _CEI_TO_STATE(_cei_, _func_)		(_cei_)->states.s_##_func_
#define _TO_CEI(_cl_)						((_CTYPE_INFO*)(_cl_))

#define _ENCODING_INFO						_Encoding_Info
#define _CTYPE_INFO							_Encoding_TypeInfo
#define _ENCODING_STATE						_Encoding_State
#define _ENCODING_MB_CUR_MAX(_ei_)			(_ei_)->mb_cur_max
#define _ENCODING_IS_STATE_DEPENDENT		0
#define _STATE_NEEDS_EXPLICIT_INIT(_ps_)	0

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

static int 	_FUNCNAME(wcrtomb_priv)(_ENCODING_INFO * __restrict, char * __restrict, size_t, wchar_t, _ENCODING_STATE * __restrict, size_t * __restrict);

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
int 		_citrus_ctype_btowc(_citrus_ctype_t * __restrict cc, int c, wint_t * __restrict wcresult);
int 		_citrus_ctype_wctob(_citrus_ctype_t * __restrict cc, wint_t wc, int * __restrict cresult);
__END_DECLS
#endif /* _CITRUS_CTYPE_H_ */

