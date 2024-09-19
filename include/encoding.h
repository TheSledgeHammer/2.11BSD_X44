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

#include <rune.h>

/* Stdenc types */
typedef uint32_t	_wc_t;
typedef uint32_t	_index_t;
typedef uint32_t 	_csid_t;

/* generic encoding structures */

typedef struct _Encoding_Charset {
	unsigned char		type;
#define	CS94			(0U)
#define	CS96			(1U)
#define	CS94MULTI		(2U)
#define	CS96MULTI		(3U)
	unsigned char		final;
	unsigned char		interm;
	unsigned char		vers;
} _Encoding_Charset;

typedef struct _Encoding_State {
	wchar_t			*ch;
	int 			chlen;
	/* UTF16, UTF32 & UES extension */
	int			current_endian;
	/* ISO2022 extension */
	_Encoding_Charset	g[4];
	int			gl:3;
	int			gr:3;
	int			singlegl:3;
	int			singlegr:3;
	int 			flags;
} _Encoding_State;

typedef struct _Encoding_Traits {
	size_t			state_size;
	size_t			mb_cur_max;
} _Encoding_Traits;

typedef struct _Encoding_Info {
	unsigned int		count[4];
	wchar_t			bits[4];
	wchar_t			mask;
	unsigned int		mb_cur_max;
	_Encoding_Traits	*traits;
	/* UTF16, UTF32 & UES extension */
	int			preffered_endian;
	uint32_t		mode;
#define _ENDIAN_UNKNOWN	0
#define _ENDIAN_BIG		1
#define _ENDIAN_LITTLE	2
#define _MODE_C99		3
	/* ISO2022 extension */
	_Encoding_Charset	*recommend[4];
	_Encoding_Charset	initg[4];
	size_t			recommendsize[4];
	int			maxcharset;
	int			flags;
#define	F_8BIT			0x0001
#define	F_NOOLD			0x0002
#define	F_SI			0x0010	/*0F*/
#define	F_SO			0x0020	/*0E*/
#define	F_LS0			0x0010	/*0F*/
#define	F_LS1			0x0020	/*0E*/
#define	F_LS2			0x0040	/*ESC n*/
#define	F_LS3			0x0080	/*ESC o*/
#define	F_LS1R			0x0100	/*ESC ~*/
#define	F_LS2R			0x0200	/*ESC }*/
#define	F_LS3R			0x0400	/*ESC |*/
#define	F_SS2			0x0800	/*ESC N*/
#define	F_SS3			0x1000	/*ESC O*/
#define	F_SS2R			0x2000	/*8E*/
#define	F_SS3R			0x4000	/*8F*/
} _Encoding_Info;

typedef struct _Encoding_TypeInfo {
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

#define _MODE_UTF32				0x00000001U
#define _MODE_FORCE_ENDIAN			0x00000002U

/*
 * macros
 */
#define _CEI_TO_EI(_cei_)			(&(_cei_)->ei)
#define _CEI_TO_STATE(_cei_, _func_)		(_cei_)->states.s_##_func_
#define _TO_CEI(_cl_)				((_CTYPE_INFO*)(_cl_))
#define _TO_EI(_cl_)				((_ENCODING_INFO*)(_cl_))
#define _TO_STATE(_ps_)				((_ENCODING_STATE*)(_ps_))

#define _ENCODING_INFO				_Encoding_Info
#define _CTYPE_INFO				_Encoding_TypeInfo
#define _ENCODING_CHARSET                   	_Encoding_Charset
#define _ENCODING_STATE				_Encoding_State
#define _ENCODING_TRAITS			_Encoding_Traits
#define _ENCODING_MB_CUR_MAX(_ei_)		(_ei_)->mb_cur_max
#define _ENCODING_IS_STATE_DEPENDENT		0
#define _STATE_NEEDS_EXPLICIT_INIT(_ps_)	0
#define _STATE_FLAG_INITIALIZED			0

#endif /* _ENCODING_H_ */
