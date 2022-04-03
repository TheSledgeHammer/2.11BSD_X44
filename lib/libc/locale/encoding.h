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

#ifndef _ENCODING_H_
#define _ENCODING_H_

#include "rune.h"

/* place in locale.h ?? */
/* generic encoding structures */
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

#endif /* _ENCODING_H_ */
