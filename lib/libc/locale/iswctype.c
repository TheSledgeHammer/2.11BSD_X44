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
#if defined(LIBC_SCCS) && !defined(lint)
__RCSID("$NetBSD: iswctype.c,v 1.14 2003/08/07 16:43:04 agc Exp $");
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>
#include <rune.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "setlocale.h"
#include "_wctrans_local.h"
#include "_wctype_local.h"

#undef iswalnum
int
iswalnum(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_A|_CTYPE_D));
}

#undef iswalpha
int
iswalpha(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_A));
}

#undef iswblank
int
iswblank(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_B));
}

#undef iswascii
int
iswascii(c)
	wint_t c;
{
	return ((c & ~0x7F) == 0);
}

#undef iswcntrl
int
iswcntrl(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_C));
}

#undef iswdigit
int
iswdigit(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_D));
}

#undef iswgraph
int
iswgraph(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_G));
}

#undef iswhexnumber
int
iswhexnumber(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_X));
}

#undef iswideogram
int
iswideogram(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_I));
}

#undef iswlower
int
iswlower(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_L));
}

#undef iswnumber
int
iswnumber(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_N));
}

#undef iswphonogram
int
iswphonogram(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_Q));
}

#undef iswprint
int
iswprint(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_R));
}

#undef iswpunct
int
iswpunct(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_P));
}

#undef iswrune
int
iswrune(c)
	wint_t c;
{
	return (__isctype_w(c, 0xFFFFFF00L));
}

#undef iswspace
int
iswspace(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_S));
}

#undef iswupper
int
iswupper(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_U));
}

#undef iswxdigit
int
iswxdigit(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_X));
}

#undef iswspecial
int
iswspecial(c)
	wint_t c;
{
	return (__isctype_w(c, _CTYPE_T));
}

#undef towupper
wint_t
towupper(c)
	wint_t c;
{
	return (__toupper_w(c));
}

#undef towlower
wint_t
towlower(c)
	wint_t c;
{
	return (__tolower_w(c));
}

int
wcwidth(c)
	wchar_t c;
{
	if (__isctype_w(c, _CTYPE_R)) {
		return (((unsigned)__runetype_w(c) & _CTYPE_SWM) >> _CTYPE_SWS);
	}
	return (-1);
}

wctrans_t
wctrans(charclass)
	const char *charclass;
{
	int i;
	_RuneLocale *rl = _RUNE_LOCALE(__get_locale());

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
towctrans(c, desc)
	wint_t c;
	wctrans_t desc;
{
	if (desc == NULL) {
		errno = EINVAL;
		return (c);
	}
	return (_towctrans(c, (_WCTransEntry*) desc));
}

wctype_t
wctype(property)
	const char *property;
{
	int i;
	_RuneLocale *rl = _RUNE_LOCALE(__get_locale());

	for (i = 0; i < _WCTYPE_NINDEXES; i++) {
		if (!strcmp(rl->wctype[i].name, property)) {
			return ((wctype_t) &rl->wctype[i]);
		}
	}
	return ((wctype_t) NULL);
}

int
iswctype(c, charclass)
	wint_t c;
	wctype_t charclass;
{
	/*
	 * SUSv3: If charclass is 0, iswctype() shall return 0.
	 */
	if (charclass == (wctype_t) 0) {
		return (0);
	}

	return (__isctype_w(c, ((_WCTypeEntry*) charclass)->mask));
}
