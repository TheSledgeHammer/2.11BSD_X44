/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * This code is derived from software contributed to Berkeley by
 * Paul Borman at Krystal Technologies.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#define _ANSI_LIBRARY
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <rune.h>

#include "setlocale.h"
#include "_ctype_local.h"

#undef isalnum_l
int
isalnum_l(c, locale)
	int c;
	locale_t locale;
{
	return(__istype_l((c), (_CTYPE_A|_CTYPE_D), locale));
}

#undef isalpha_l
int
isalpha_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_A, locale));
}

#undef isascii_l
int
isascii_l(c, locale)
	int c;
	locale_t locale;
{
	return ((c & ~0x7F) == 0);
}

#undef isblank_l
int
isblank_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_B, locale));
}

#undef iscntrl_l
int
iscntrl_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_C, locale));
}

#undef isdigit_l
int
isdigit_l(c, locale)
	int c;
	locale_t locale;
{
	return (__isctype_l((c), _CTYPE_D, locale));
}

#undef isgraph_l
int
isgraph_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_G, locale));
}

#undef ishexnumber_l
int
ishexnumber_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l(c, _CTYPE_X, locale));
}

#undef isideogram_l
int
isideogram_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_I, locale));
}

#undef islower_l
int
islower_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_L, locale));
}

#undef isnumber_l
int
isnumber_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l(c, _CTYPE_N, locale));
}

#undef isphonogram_l
int
isphonogram_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_Q, locale));
}

#undef isprint_l
int
isprint_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_R, locale));
}

#undef ispunct_l
int
ispunct_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_P, locale));
}

#undef isrune_l
int
isrune_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), 0xFFFFFF00L, locale));
}

#undef isspace_l
int
isspace_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_S, locale));
}

#undef isspecial_l
int
isspecial_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_T, locale));
}

#undef isupper_l
int
isupper_l(c, locale)
	int c;
	locale_t locale;
{
	return (__istype_l((c), _CTYPE_U, locale));
}

#undef isxdigit_l
int
isxdigit_l(c, locale)
	int c;
	locale_t locale;
{
	return (__isctype_l((c), _CTYPE_X, locale));
}

#undef toascii_l
int
toascii_l(c, locale)
	int c;
	locale_t locale;
{
	return (c & 0177);
}

#undef tolower_l
int
tolower_l(c, locale)
	int c;
	locale_t locale;
{
	return (__tolower_l(c, locale));
}

#undef toupper_l
int
toupper_l(c, locale)
	int c;
	locale_t locale;
{
	return (__toupper_l(c, locale));
}
