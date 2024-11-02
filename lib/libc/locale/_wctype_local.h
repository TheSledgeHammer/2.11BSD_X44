/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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

#ifndef _WCTYPE_LOCAL_H_
#define _WCTYPE_LOCAL_H_

#include "setlocale.h"
#include "_wctrans_local.h"

#define _RUNE_LOCALE(loc) ((_RuneLocale *)((loc)->part_impl[(size_t)LC_CTYPE]))

static __inline _RuneType
__runetype_wl(c, locale)
	wint_t c;
	locale_t locale;
{
	_RuneLocale *rl = _RUNE_LOCALE(locale);
	return (_CRMASK(c) ? ___runetype_mb(c) : rl->runetype[c]);
}

static __inline int
__isctype_wl(c, f, locale)
	wint_t c;
	_RuneType f;
	locale_t locale;
{
	return (!!(__runetype_wl(c, locale) & f));
}

static __inline wint_t
__toupper_wl(c, locale)
	wint_t c;
	locale_t locale;
{
	_RuneLocale *rl = _RUNE_LOCALE(locale);
	return (_towctrans(c, _wctrans_upper(rl)));
}

static __inline wint_t
__tolower_wl(c, locale)
	wint_t c;
	locale_t locale;
{
	_RuneLocale *rl = _RUNE_LOCALE(locale);
	return (_towctrans(c, _wctrans_lower(rl)));
}

static __inline _RuneType
__runetype_w(c)
	wint_t c;
{
	return (__runetype_wl(c, __get_locale()));
}

static __inline int
__isctype_w(c, f)
	wint_t c;
	_RuneType f;
{
	return (__isctype_wl(c, f, __get_locale()));
}

static __inline wint_t
__toupper_w(c)
	wint_t c;
{
	return (__toupper_wl(c, __get_locale()));
}

static __inline wint_t
__tolower_w(c)
	wint_t c;
{
	return (__tolower_wl(c, __get_locale()));
}

#endif /* _WCTYPE_LOCAL_H_ */
