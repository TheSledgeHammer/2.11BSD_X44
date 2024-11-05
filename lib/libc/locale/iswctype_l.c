/*	$NetBSD: iswctype.c,v 1.14 2003/08/07 16:43:04 agc Exp $	*/

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
#include <sys/cdefs.h>

#include "namespace.h"

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <rune.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "setlocale.h"
#include "_wctrans_local.h"
#include "_wctype_local.h"

#undef iswalnum_l
int
iswalnum_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_A|_CTYPE_D, locale));
}

#undef iswalpha_l
int
iswalpha_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_A, locale));
}

#undef iswblank_l
int
iswblank_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_B, locale));
}

#undef iswascii_l
int
iswascii_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return ((c & ~0x7F) == 0);
}

#undef iswcntrl_l
int
iswcntrl_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_C, locale));
}

#undef iswdigit_l
int
iswdigit_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_D, locale));
}

#undef iswgraph_l
int
iswgraph_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_G, locale));
}

#undef iswhexnumber_l
int
iswhexnumber_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_X, locale));
}

#undef iswideogram_l
int
iswideogram_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_I, locale));
}

#undef iswlower_l
int
iswlower_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_L, locale));
}

#undef iswnumber_l
int
iswnumber_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl(c, _CTYPE_N, locale));
}

#undef iswphonogram_l
int
iswphonogram_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl(c, _CTYPE_Q, locale));
}

#undef iswprint_l
int
iswprint_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_R, locale));
}

#undef iswpunct_l
int
iswpunct_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_P, locale));
}

#undef iswrune_l
int
iswrune_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl(c, 0xFFFFFF00L, locale));
}

#undef iswspace_l
int
iswspace_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_S, locale));
}

#undef iswupper_l
int
iswupper_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_U, locale));
}

#undef iswxdigit_l
int
iswxdigit_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_X, locale));
}

#undef iswspecial_l
int
iswspecial_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__isctype_wl((c), _CTYPE_T, locale));
}

#undef towupper_l
wint_t
towupper_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__toupper_wl(c, locale));
}

#undef towlower_l
wint_t
towlower_l(c, locale)
	wint_t c;
	locale_t locale;
{
	return (__tolower_wl(c, locale));
}

int
wcwidth_l(c, locale)
	wchar_t c;
	locale_t locale;
{
	if (__isctype_wl(c, _CTYPE_R, locale)) {
		return (((unsigned)__runetype_wl(c, locale) & _CTYPE_SWM) >> _CTYPE_SWS);
	}
	return (-1);
}

wctrans_t
wctrans_l(charclass, locale)
	const char *charclass;
	locale_t locale;
{
	_RuneLocale *rl;
	int i;

	rl = _RUNE_LOCALE(locale);
	if (rl->wctrans[_WCTRANS_INDEX_LOWER].name == NULL) {
		wctrans_init(rl);
	}

	for (i = 0; i < _WCTRANS_NINDEXES; i++) {
		if (!strcmp(rl->wctrans[i].name, charclass)) {
			return ((wctrans_t) &rl->wctrans[i]);
		}
	}
	return ((wctrans_t) NULL);
}

wint_t
towctrans_l(c, desc, locale)
	wint_t c;
	wctrans_t desc;
	locale_t locale;
{
	return (towctrans(c, desc));
}

wctype_t
wctype_l(property, locale)
	const char *property;
	locale_t locale;
{
	_RuneLocale *rl;
	int i;

	rl = _RUNE_LOCALE(locale);
	for (i = 0; i < _WCTYPE_NINDEXES; i++) {
		if (!strcmp(rl->wctype[i].name, property)) {
			return ((wctype_t) &rl->wctype[i]);
		}
	}
	return ((wctype_t) NULL);
}

int
iswctype_l(c, charclass, locale)
	wint_t c;
	wctype_t charclass;
	locale_t locale;
{
	/*
	 * SUSv3: If charclass is 0, iswctype() shall return 0.
	 */
	if (charclass == (wctype_t) 0) {
		return (0);
	}

	return (__isctype_wl(c, ((_WCTypeEntry*) charclass)->mask, locale));
}
