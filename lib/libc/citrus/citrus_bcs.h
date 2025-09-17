/*	$NetBSD: citrus_bcs.h,v 1.2 2004/01/02 21:49:35 itojun Exp $	*/

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


#ifndef _CITRUS_BCS_H_
#define _CITRUS_BCS_H_

#include "aliasname_local.h"

#define _citrus_bcs_skip_ws(ch)					_bcs_skip_ws(ch)
#define _citrus_bcs_skip_nonws(ch)				_bcs_skip_nonws(ch)
#define _citrus_bcs_skip_ws_len(ch, len)		_bcs_skip_ws_len(ch, len)
#define _citrus_bcs_skip_nonws_len(ch, len)		_bcs_skip_nonws_len(ch, len)
#define _citrus_bcs_trunc_rws_len(ch, len)		_bcs_trunc_rws_len(ch, len)
#define _citrus_bcs_convert_to_lower(ch)		_bcs_convert_to_lower(ch)
#define _citrus_bcs_convert_to_upper(ch)		_bcs_convert_to_upper(ch)
#define _citrus_bcs_ignore_case(ic, ch)			_bcs_ignore_case(ic, ch)
#define _citrus_bcs_is_ws(ch)					_bcs_is_ws(ch)

#define _citrus_bcs_isblank(ch)					_bcs_isblank(ch)
#define _citrus_bcs_iseol(ch)					_bcs_iseol(ch)
#define _citrus_bcs_isspace(ch) 				_bcs_isspace(ch)
#define _citrus_bcs_isdigit(ch) 				_bcs_isdigit(ch)
#define _citrus_bcs_isupper(ch) 				_bcs_isupper(ch)
#define _citrus_bcs_islower(ch) 				_bcs_islower(ch)
#define _citrus_bcs_isalpha(ch) 				_bcs_isalpha(ch)
#define _citrus_bcs_isalnum(ch) 				_bcs_isalnum(ch)
#define _citrus_bcs_isxdigit(ch) 				_bcs_isxdigit(ch)
#define _citrus_bcs_toupper(ch) 				_bcs_toupper(ch)
#define _citrus_bcs_tolower(ch) 				_bcs_tolower(ch)
#define _citrus_bcs_strcasecmp(str1, str2)			_bcs_strcasecmp(str1, str2)
#define _citrus_bcs_strncasecmp(str1, str2, size)	_bcs_strncasecmp(str1, str2, size)

#endif /* _CITRUS_BCS_H_ */
