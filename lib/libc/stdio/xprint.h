/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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

#ifndef _XPRINT_H_
#define _XPRINT_H_

#ifndef NARROW
#define CHAR_T  wchar_t
#else
#define CHAR_T  char
#endif

/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	(CHAR_T)((n) + '0')

#ifdef FLOATING_POINT

union fparg {
	double dbl;
	long double ldbl;
};

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */
#define	DEFPREC	6

int grouping_init(int, int, int, const char *);
char *cvt(union fparg *, const char *, int, int, CHAR_T *, int *, int, int *);
int exponent(CHAR_T *, int, int);

#else /* no FLOATING_POINT */

#define	BUF		100

#endif /* FLOATING_POINT */

/*
 * Flags used during conversion.
 */
#define	ALT			0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	LLONGINT	QUADINT		/* long long integer */
#define	SHORTINT	0x040		/* short integer */
#define	MAXINT		0x080		/* (unsigned) intmax_t */
#define	PTRDIFFT	0x100		/* (unsigned) ptrdiff_t */
#define	SIZEINT		0x200		/* (signed) size_t */
#define	CHARINT     0x400		/* print char using int format */
#define	ZEROPAD		0x800		/* zero (as opposed to blank) pad */
#define FPT			0x1000		/* Floating point number */
#define	GROUPING	0x2000		/* use grouping ("'" flag) */

CHAR_T *__ultoa(u_long, const CHAR_T *, int, int, const char *, int, char, const char *);
CHAR_T *__uqtoa(u_quad_t, const CHAR_T *, int, int, const char *, int, char, const char *);
CHAR_T *__ujtoa(uintmax_t, const CHAR_T *, int, int, const char *, int, char, const char *);

#endif /* _XPRINT_H_ */
