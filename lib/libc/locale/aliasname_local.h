/* $NetBSD: aliasname_local.h,v 1.1 2002/02/13 07:45:52 yamt Exp $ */

/*-
 * Copyright (c)2002 YAMAMOTO Takashi,
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

#ifndef _ALIASNAME_LOCAL_H_
#define _ALIASNAME_LOCAL_H_

/*
 * predicate/conversion for basic character set.
 */

#define _BCS_PRED(_name_, _cond_) \
static __inline int _bcs_##_name_(u_int8_t ch) { return (_cond_); }

_BCS_PRED(isblank, ch == ' ' || ch == '\t')
_BCS_PRED(iseol, ch == '\n' || ch == '\r')
_BCS_PRED(isspace, _bcs_isblank(ch) || _bcs_iseol(ch) || ch == '\f' || ch == '\v')
_BCS_PRED(isdigit, ch >= '0' && ch <= '9')
_BCS_PRED(isupper, ch >= 'A' && ch <= 'Z')
_BCS_PRED(islower, ch >= 'a' && ch <= 'z')
_BCS_PRED(isalpha, _bcs_isupper(ch) || _bcs_islower(ch))
_BCS_PRED(isalnum, _bcs_isdigit(ch) || _bcs_isalpha(ch))
_BCS_PRED(isxdigit, _bcs_isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f'))

static __inline u_int8_t
_bcs_toupper(u_int8_t ch)
{
	return (_bcs_islower(ch) ? (ch - 'a' + 'A') : ch);
}

static __inline u_int8_t
_bcs_tolower(u_int8_t ch)
{
	return (_bcs_isupper(ch) ? (ch - 'A' + 'a') : ch);
}

/* citrus_bcs */
__BEGIN_DECLS
int		_bcs_strcasecmp(const char *, const char *);
int		_bcs_strncasecmp(const char *, const char *, size_t);
const char 	*_bcs_skip_ws(const char *);
const char 	*_bcs_skip_nonws(const char *);
const char 	*_bcs_skip_ws_len(const char *, size_t *);
const char 	*_bcs_skip_nonws_len(const char *, size_t *);
void 	 	_bcs_trunc_rws_len(const char *, size_t *);
void		_bcs_convert_to_lower(char *);
void		_bcs_convert_to_upper(char *);
void		_bcs_ignore_case(int, char *);
int        	_bcs_is_ws(const char);
const char 	*__unaliasname(const char *, const char *, void *, size_t);
int 		__isforcemapping(const char *);
__END_DECLS
#endif /*_ALIASNAME_LOCAL_H_*/
