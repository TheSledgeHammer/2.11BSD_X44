/*	$NetBSD: wchar.h,v 1.19 2003/07/08 05:39:23 itojun Exp $	*/

/*-
 * Copyright (c)1999 Citrus Project,
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

/*-
 * Copyright (c) 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Julian Coleman.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _WCHAR_H_
#define _WCHAR_H_

#include <sys/cdefs.h>
#include <machine/ansi.h>
#include <sys/ansi.h>
#include <sys/null.h>

#include <stdio.h> /* for FILE* */

#ifdef	_BSD_WCHAR_T_
typedef	_BSD_WCHAR_T_	wchar_t;
#undef	_BSD_WCHAR_T_
#endif

#ifdef	_BSD_MBSTATE_T_
typedef	_BSD_MBSTATE_T_	mbstate_t;
#undef	_BSD_MBSTATE_T_
#endif

#ifdef	_BSD_WINT_T_
typedef	_BSD_WINT_T_	wint_t;
#undef	_BSD_WINT_T_
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifndef WEOF
#define	WEOF 			((wint_t)-1)
#endif

#define getwc(f) 		fgetwc(f)
#define getwchar() 		getwc(stdin)
#define putwc(wc, f) 	fputwc((wc), (f))
#define putwchar(wc) 	putwc((wc), stdout)

__BEGIN_DECLS
wint_t	btowc(int);
size_t	mbrlen(const char * __restrict, size_t, mbstate_t * __restrict);
size_t	mbrtowc(wchar_t * __restrict, const char * __restrict, size_t, mbstate_t * __restrict);
int		mbsinit(const mbstate_t *);
size_t	mbsrtowcs(wchar_t * __restrict, const char ** __restrict, size_t, mbstate_t * __restrict);
size_t	wcrtomb(char * __restrict, wchar_t, mbstate_t * __restrict);
wchar_t	*wcscat(wchar_t * __restrict, const wchar_t * __restrict);
wchar_t	*wcschr(const wchar_t *, wchar_t);
int		wcscmp(const wchar_t *, const wchar_t *);
int		wcscoll(const wchar_t *, const wchar_t *);
wchar_t	*wcscpy(wchar_t * __restrict, const wchar_t * __restrict);
size_t	wcscspn(const wchar_t *, const wchar_t *);
size_t	wcslen(const wchar_t *);
wchar_t	*wcsncat(wchar_t * __restrict, const wchar_t * __restrict, size_t);
int		wcsncmp(const wchar_t *, const wchar_t *, size_t);
wchar_t	*wcsncpy(wchar_t * __restrict , const wchar_t * __restrict, size_t);
wchar_t	*wcspbrk(const wchar_t *, const wchar_t *);
wchar_t	*wcsrchr(const wchar_t *, wchar_t);
size_t	wcsrtombs(char * __restrict, const wchar_t ** __restrict, size_t, mbstate_t * __restrict);
size_t	wcsspn(const wchar_t *, const wchar_t *);
wchar_t	*wcsstr(const wchar_t *, const wchar_t *);
wchar_t *wcstok(wchar_t * __restrict, const wchar_t * __restrict, wchar_t ** __restrict);
size_t	wcsxfrm(wchar_t *, const wchar_t *, size_t);
wchar_t	*wcswcs(const wchar_t *, const wchar_t *);
wchar_t	*wmemchr(const wchar_t *, wchar_t, size_t);
int		  wmemcmp(const wchar_t *, const wchar_t *, size_t);
wchar_t	*wmemcpy(wchar_t * __restrict, const wchar_t * __restrict, size_t);
wchar_t	*wmemmove(wchar_t *, const wchar_t *, size_t);
wchar_t	*wmemset(wchar_t *, wchar_t, size_t);

#if defined(__BSD_VISIBLE)
size_t	wcslcat(wchar_t *, const wchar_t *, size_t);
size_t	wcslcpy(wchar_t *, const wchar_t *, size_t);
#endif

#if defined(__BSD_VISIBLE) || (_POSIX_C_SOURCE - 0 >= 200112L) || (_XOPEN_SOURCE - 0) >= 600
int		wcswidth(const wchar_t *, size_t);
int		wcwidth(wchar_t);
#endif

int		wctob(wint_t);
unsigned long int wcstoul(const wchar_t * __restrict, wchar_t ** __restrict, int);
long int wcstol(const wchar_t * __restrict, wchar_t ** __restrict, int);
double wcstod(const wchar_t * __restrict, wchar_t ** __restrict);

#if defined(_ISOC99_SOURCE) || (__STDC_VERSION__ - 0) > 199901L || defined(__BSD_VISIBLE)
float wcstof(const wchar_t * __restrict, wchar_t ** __restrict);
long double wcstold(const wchar_t * __restrict, wchar_t ** __restrict);

/* LONGLONG */
long long int wcstoll(const wchar_t * __restrict, wchar_t ** __restrict, int);
/* LONGLONG */
unsigned long long int wcstoull(const wchar_t * __restrict, wchar_t ** __restrict, int);
#endif

#if (_POSIX_C_SOURCE - 0) >= 200809L || (_XOPEN_SOURCE - 0) >= 700 || \
    defined(__BSD_VISIBLE)
FILE	*open_wmemstream(wchar_t **, size_t *);
#endif

wint_t 	ungetwc(wint_t, FILE *);
wint_t 	fgetwc(FILE *);
wchar_t *fgetws(wchar_t * __restrict, int, FILE * __restrict);
wint_t 	getwc(FILE *);
//wint_t 	getwchar(void);
wint_t 	fputwc(wchar_t, FILE *);
int 	  fputws(const wchar_t * __restrict, FILE * __restrict);
//wint_t 	putwc(wchar_t, FILE *);
//wint_t 	putwchar(wchar_t);
int 	  fwide (FILE *, int);

wchar_t     *wcsdup(const wchar_t *);

#if (_POSIX_C_SOURCE - 0) >= 200809L || defined(__BSD_VISIBLE)
#  ifndef __LOCALE_T_DECLARED
//typedef void		*locale_t;
typedef struct _locale *locale_t;
#  define __LOCALE_T_DECLARED
#  endif

int	    wcwidth_l(wchar_t, locale_t);
#endif /* _POSIX_C_SOURCE >= 200809 || __BSD_VISIBLE */

#if defined(__BSD_VISIBLE)

wint_t	btowc_l(int, locale_t);
size_t	mbrlen_l(const char * __restrict, size_t, mbstate_t * __restrict, locale_t);
size_t	mbrtowc_l(wchar_t * __restrict, const char * __restrict, size_t, mbstate_t * __restrict, locale_t);
int		  mbsinit_l(const mbstate_t *, locale_t);
size_t	mbsrtowcs_l(wchar_t * __restrict, const char ** __restrict, size_t, mbstate_t * __restrict, locale_t);
size_t	mbsnrtowcs_l(wchar_t * __restrict, const char ** __restrict, size_t, size_t, mbstate_t * __restrict, locale_t);
size_t	wcrtomb_l(char * __restrict, wchar_t, mbstate_t * __restrict, locale_t);
size_t	wcsrtombs_l(char * __restrict, const wchar_t ** __restrict, size_t, mbstate_t * __restrict, locale_t);
int		  wctob_l(wint_t, locale_t);
#endif /* __BSD_VISIBLE */
__END_DECLS

#endif /* !_WCHAR_H_ */
