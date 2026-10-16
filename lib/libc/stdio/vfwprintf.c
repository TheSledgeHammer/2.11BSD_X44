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

/*	$NetBSD: vfwprintf.c,v 1.42 2026/09/21 19:12:51 kre Exp $	*/
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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)vfprintf.c	5.2 (Berkeley) 6/27/88";
static char sccsid[] = "@(#)vfprintf.c	8.1 (Berkeley) 6/4/93";
__FBSDID("$FreeBSD: src/lib/libc/stdio/vfwprintf.c,v 1.27 2007/01/09 00:28:08 imp Exp $");
//#else
__RCSID("$NetBSD: vfwprintf.c,v 1.42 2026/09/21 19:12:51 kre Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

/*
 * Actual {w,}printf innards.
 */

#include "namespace.h"
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <locale.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>

#include "reentrant.h"
#include "setlocale.h"
#include "local.h"
#include "fvwrite.h"
#include "extern.h"

#include "xprint.h"

static int __sprint(FILE *, locale_t, struct __suio *);
static int __sbprintf(FILE *, locale_t, const wchar_t *, va_list);
static wint_t __xfputwc(wchar_t, FILE *, locale_t);
static wchar_t *__mbsconv(char *, int, locale_t);
int vfwprintf_l_unlocked(FILE *, locale_t, const wchar_t *, va_list);

static const char xdigs_lower[16] = "0123456789abcdef";
static const char xdigs_upper[16] = "0123456789ABCDEF";

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 *
 * XXX The fact that we do this a character at a time and convert to a
 * multibyte character sequence even if the destination is a wide
 * string eclipses the benefits of buffering.
 */
static int
__sprint(FILE *fp, locale_t locale, struct __suio *uio)
{
	struct __siov *iov;
	wchar_t *p;
	int i, len;

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(uio != NULL);

	if (uio->uio_resid == 0) {
		uio->uio_iovcnt = 0;
		return (0);
	}

	iov = uio->uio_iov;
	for (; uio->uio_resid != 0; uio->uio_resid -= len, iov++) {
		p = (wchar_t *)iov->iov_base;
		len = iov->iov_len;
		for (i = 0; i < len; i++) {
			if (__xfputwc(p[i], fp, locale) == WEOF) {
				return (-1);
			}
		}
	}
	uio->uio_resid = 0;
	uio->uio_iovcnt = 0;
	return (0);
}

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
__sbprintf(FILE *fp, locale_t locale, const wchar_t *fmt, va_list ap)
{
	int ret;
	FILE fake;
    struct __sfileext fakeext;
	unsigned char buf[BUFSIZ];

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(fmt != NULL);

    _FILEEXT_SETUP(&fake, &fakeext);
    memset(WCIO_GET(&fake), 0, sizeof(struct wchar_io_data));

	/* copy the important variables */
	fake._flags = fp->_flags & ~__SNBF;
	fake._file = fp->_file;
	fake._cookie = fp->_cookie;
	fake._write = fp->_write;

	/* set up the buffer */
	fake._bf._base = fake._p = buf;
	fake._bf._size = fake._w = sizeof(buf);
	fake._lbfsize = 0; /* not actually used, but Just In Case */

	/* do the work, then copy any error status */
	ret = vfwprintf_l_unlocked(&fake, locale, fmt, ap);
	if (ret >= 0 && fflush(&fake))
		ret = WEOF;
	if (fake._flags & __SERR)
		fp->_flags |= __SERR;
	return (ret);
}

/*
 * Like __fputwc, but handles fake string (__SSTR) files properly.
 * File must already be locked.
 */
static wint_t
__xfputwc(wchar_t wc, FILE *fp, locale_t locale)
{
	static const mbstate_t initial;
	mbstate_t mbs;
	char buf[MB_LEN_MAX];
	struct __suio uio;
	struct __siov iov;
	size_t len;

	if ((fp->_flags & __SSTR) == 0) {
		return (fputwc(wc, fp));
	}

	mbs = initial;
	if ((len = wcrtomb_l(buf, wc, &mbs, locale)) == (size_t)-1) {
		fp->_flags |= __SERR;
		return (WEOF);
	}
	uio.uio_iov = &iov;
	uio.uio_resid = len;
	uio.uio_iovcnt = 1;
	iov.iov_base = buf;
	iov.iov_len = len;
	return (__sfvwrite(fp, &uio) != EOF ? (wint_t)wc : WEOF);
}

/*
 * Convert a multibyte character string argument for the %s format to a wide
 * string representation. ``prec'' specifies the maximum number of bytes
 * to output. If ``prec'' is greater than or equal to zero, we can't assume
 * that the multibyte char. string ends in a null character.
 */
static wchar_t *
__mbsconv(char *mbsarg, int prec, locale_t locale)
{
	static const mbstate_t initial;
	mbstate_t mbs;
	wchar_t *convbuf, *wcp;
	const char *p;
	size_t insize, nchars, nconv;

	if (mbsarg == NULL)
		return (NULL);

	/*
	 * Supplied argument is a multibyte string; convert it to wide
	 * characters first.
	 */
	if (prec >= 0) {
		/*
		 * String is not guaranteed to be NUL-terminated. Find the
		 * number of characters to print.
		 */
		p = mbsarg;
		insize = nchars = nconv = 0;
		mbs = initial;
		while (nchars != (size_t)prec) {
			nconv = mbrlen_l(p, MB_CUR_MAX, &mbs, locale);
			if (nconv == 0 || nconv == (size_t)-1 || nconv == (size_t)-2)
				break;
			p += nconv;
			nchars++;
			insize += nconv;
		}
		if (nconv == (size_t)-1 || nconv == (size_t)-2)
			return (NULL);
	} else
		insize = strlen(mbsarg);

	/*
	 * Allocate buffer for the result and perform the conversion,
	 * converting at most `size' bytes of the input multibyte string to
	 * wide characters for printing.
	 */
	convbuf = NULL;
	errno = reallocarr(&convbuf, insize + 1, sizeof(*convbuf));
	if (errno)
		return (NULL);
	wcp = convbuf;
	p = mbsarg;
	mbs = initial;
	nconv = 0;
	while (insize != 0) {
		nconv = mbrtowc_l(wcp, p, insize, &mbs, locale);
		if (nconv == 0 || nconv == (size_t)-1 || nconv == (size_t)-2)
			break;
		wcp++;
		p += nconv;
		insize -= nconv;
	}
	if (nconv == (size_t)-1 || nconv == (size_t)-2) {
		int serrno = errno;
		free(convbuf);
		errno = serrno;
		return (NULL);
	}
	*wcp = L'\0';

	return (convbuf);
}

#ifdef FLOATING_POINT
#include <locale.h>
#include <math.h>
#include "floatio.h"
#endif /* FLOATING_POINT */

int
vfwprintf(FILE *fp, const wchar_t *fmt0, va_list ap)
{
	return (vfwprintf_l(fp, __get_locale(), fmt0, ap));
}

int
vfwprintf_l(FILE *fp, locale_t locale, const wchar_t *fmt0, va_list ap)
{
	int ret;

	FLOCKFILE(fp);
	ret = vfwprintf_l_unlocked(fp, locale, fmt0, ap);
	FUNLOCKFILE(fp);
	return (ret);
}

int
vfwprintf_l_unlocked(FILE *fp, locale_t locale, const wchar_t *fmt0, va_list ap)
{
	const wchar_t *fmt; 	/* format string */
	int ch; 				/* character from fmt */
	int m, n; 				/* handy integers (short term usage) */
	const wchar_t *cp; 		/* handy char pointer (short term usage) */
	const wchar_t *bp; 		/* handy char pointer (short term usage) */
	int flags; 				/* flags as above */
	int ret; 				/* return value accumulator */
	int width; 				/* width from format (%8d), or 0 */
	int prec; 				/* precision from format (%.3d), or -1 */
	wchar_t sign; 			/* sign prefix (' ', '+', '-', or \0) */
	char thousands_sep; 	/* locale specific thousands separator */
	const char *grouping; 	/* locale specific numeric grouping rules */
#ifdef FLOATING_POINT
	char *decimal_point;	/* locale specific decimal point */
	wchar_t softsign;		/* temporary negative sign for floats */
	union fparg fparg;		/* double precision arguments %[aAeEfgG] */
	int expt;				/* integer value of exponent */
	int expsize;			/* character count for expstr */
	int ndig;				/* actual number of digits returned by cvt */
	char *dtoaresult;		/* buffer allocated by dtoa */
	wchar_t expstr[MAXEXPDIG+2]; /* buffer for exponent string */
	int nseps;				/* number of group separators with ' */
	int nrepeats;			/* number of repeats of the last group */
	int n2;
#endif
	u_long ulval;			/* integer arguments %[diouxX] */
	uintmax_t ujval;		/* %j, %ll, %q, %t, %z integers */
	int base; 				/* base for [diouxX] conversion */
	int dprec;				/* a copy of prec if [diouxX], 0 otherwise */
	int fieldsz;			/* field size expanded by sign, etc */
	int realsz;				/* field size expanded by dprec */
	int size;				/* size of converted field or string */
	int prsize;             /* max size of printed field */
	const char *xdigs; 		/* digits for %[xX] conversion */
#define NIOV 8
	struct __suio uio;		/* output information: summary */
	struct __siov iov[NIOV];/* ... and individual io vectors */
	struct __siov *iovp;	/* for PRINT macro */
	wchar_t buf[BUF];		/* space for %c, %[diouxX], %[eEfgG] */
	wchar_t ox[2]; 			/* space for 0x hex-prefix */
	wchar_t *convbuf; 		/* multibyte to wide conversion result */

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static const wchar_t blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};

	static const wchar_t zeroes[PADSIZE] =
	{'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};

#define	PRINT(ptr, len)	do {				\
	iovp->iov_base = __UNCONST(ptr); 		\
	iovp->iov_len = (len); 					\
	uio.uio_resid += (len); 				\
	iovp++; 								\
	if (++uio.uio_iovcnt >= NIOV) { 		\
		if (__sprint(fp, locale, &uio))	{	\
			goto error;						\
		}									\
		iovp = iov;							\
	}										\
} while (0)
#define	PAD(howmany, with)	do {	\
	if ((n = (howmany)) > 0) {		\
		while (n > PADSIZE) {		\
			PRINT(with, PADSIZE);	\
			n -= PADSIZE;			\
		}							\
		PRINT(with, n);				\
	}								\
} while (0)
#define	FLUSH() do { 						\
	if (uio.uio_resid && 					\
			__sprint(fp, locale, &uio)) {	\
		goto error;							\
	}										\
	uio.uio_iovcnt = 0; 					\
	iovp = iov;								\
} while (0)
#ifdef FLOATING_POINT
#define	PRINTANDPAD(p, ep, len, with) do {	\
	ptrdiff_t td = (ep) - (p);				\
	n2 = (int)td;       					\
	if (n2 > (len))							\
		n2 = (len);							\
	if (n2 > 0)								\
		PRINT((p), n2);						\
	PAD((len) - (n2 > 0 ? n2 : 0), (with));	\
} while (0)
#endif

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() 												\
	(flags&LONGINT ? va_arg(ap, long) : 					\
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : 	\
	    flags&CHARINT ? (long)(char)va_arg(ap, int) : 		\
	    (long)va_arg(ap, int))
#define	UARG() 												\
	(flags&LONGINT ? va_arg(ap, u_long) : 					\
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    flags&CHARINT ? (u_long)(u_char)va_arg(ap, int) : 	\
	    (u_long)va_arg(ap, u_int))
#define	INTMAX_SIZE	(MAXINT|SIZEINT|PTRDIFFT|LLONGINT)
#define SJARG() 											\
	(flags&MAXINT ? va_arg(ap, intmax_t) : 					\
	    flags&SIZEINT ? (intmax_t)va_arg(ap, ssize_t) : 	\
	    flags&PTRDIFFT ? (intmax_t)va_arg(ap, ptrdiff_t) : \
	    (intmax_t)va_arg(ap, quad_t))
#define	UJARG() 											\
	(flags&MAXINT ? va_arg(ap, uintmax_t) : 				\
	    flags&SIZEINT ? (uintmax_t)va_arg(ap, size_t) : 	\
	    flags&PTRDIFFT ? (uintmax_t)va_arg(ap, ptrdiff_t) : \
	    (uintmax_t)va_arg(ap, u_quad_t))

	/*
	 * Append a digit to a value and check for overflow.
	 */
#define APPEND_DIGIT(val, dig) do { 		\
	if ((val) > INT_MAX / 10) 				\
		errno = EOVERFLOW;					\
		ret = WEOF;							\
		goto error; 						\
	(val) *= 10; 							\
	if ((val) > INT_MAX - to_digit((dig))) 	\
		errno = EOVERFLOW;					\
		ret = WEOF;							\
		goto error; 						\
	(val) += to_digit((dig)); 				\
} while (0)

	/* Check for hexprefix */
#define ISHEX(flags) \
	((flags & HEXPREFIX) || ox[1])

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(fmt0 != NULL);

	_SET_ORIENTATION(fp, -1);

	thousands_sep = '\0';
	grouping = NULL;
#ifdef FLOATING_POINT
	decimal_point = localeconv_l(locale)->decimal_point;
	expsize = 0;
	nseps = nrepeats = 0;
#endif

	convbuf = NULL;
	/* sorry, fprintf(read_only_file, "") returns WEOF, not 0 */
	if (cantwrite(fp)) {
		return (WEOF);
	}

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
	if ((fp->_flags & (__SNBF | __SWR | __SRW)) == (__SNBF | __SWR)
			&& fp->_file >= 0) {
		ret = __sbprintf(fp, locale, fmt0, ap);
		return (ret);
	}

	fmt = fmt0;
	uio.uio_iov = iovp = iov;
	uio.uio_resid = 0;
	uio.uio_iovcnt = 0;
	ret = 0;
	ulval = 0;
	ujval = 0;
	xdigs = NULL;

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		for (cp = fmt; (ch = *fmt) != '\0' && ch != '%'; fmt++) {
			/* void */;
		}
		m = fmt - cp;
		if (m != 0) {
			if (m < 0 || (unsigned)m + ret > INT_MAX) {
				errno = EOVERFLOW;
				ret = WEOF;
				goto error;
			}
			PRINT(cp, m);
			ret += m;
		}
		n = m;
		if (ch == '\0')
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';
		ox[1] = '\0';

rflag:
		ch = *fmt++;
reswitch:
		switch (ch) {
		case ' ':
			/*
			 * ``If the space and + flags both appear, the space
			 * flag will be ignored.''
			 *	-- ANSI X3J11
			 */
			if (!sign)
				sign = ' ';
			goto rflag;
		case '#':
			flags |= ALT;
			goto rflag;
		case '*':
			/*
			 * ``A negative field width argument is taken as a
			 * - flag followed by a positive field width.''
			 *	-- ANSI X3J11
			 * They don't exclude field widths read from args.
			 */
			width = va_arg(ap, int);
			if (width >= 0)
				goto rflag;
			if (width == INT_MIN) {
				errno = EOVERFLOW;
				ret = WEOF;
				goto error;
			}
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '\'':
			flags |= GROUPING;
			thousands_sep = *(localeconv_l(locale)->thousands_sep);
			grouping = localeconv_l(locale)->grouping;
			/* Use grouping if defined by locale */
			if (thousands_sep && grouping && *grouping)
				flags |= GROUPING;
			else {
				thousands_sep = '\0';
				grouping = NULL;
			}
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				APPEND_DIGIT(n, ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*-
			 * ``Note that 0 is taken as a flag, not as the
			 * beginning of a field width.''
			 *	-- ANSI X3J11
			 */
			flags |= ZEROPAD;
			goto rflag;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			n = 0;
			do {
				APPEND_DIGIT(n, ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif /* FLOATING_POINT */
		case 'h':
			if (flags & SHORTINT) {
				flags &= ~SHORTINT;
				flags |= CHARINT;
			} else
				flags |= SHORTINT;
			goto rflag;
		case 'j':
			flags |= MAXINT;
			goto rflag;
		case 'l':
			if (flags & LONGINT) {
				flags &= ~LONGINT;
				flags |= LLONGINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= LLONGINT;	/* not necessarily */
			goto rflag;
		case 't':
			flags |= PTRDIFFT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'C':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'c':
			if (flags & LONGINT) {
				*buf = (wchar_t)va_arg(ap, wint_t);
			} else {
				*buf = (wchar_t)btowc_l(va_arg(ap, int), locale);
			}
            cp = buf;
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
			if (flags & INTMAX_SIZE) {
				ujval = SJARG();
				if ((intmax_t)ujval < 0) {
					ujval = -ujval;
					sign = '-';
				}
			} else {
				ulval = SARG();
				if ((long)ulval < 0) {
					ulval = -ulval;
					sign = '-';
				}
			}
			base = 10;
			goto number;
		case 'i':
#ifdef FLOATING_POINT
		case 'a':
		case 'A':
			if (ch == 'a') {
				ox[1] = 'x';
				xdigs = xdigs_lower;
			} else {
				ox[1] = 'X';
				xdigs = xdigs_upper;
			}
			if (prec >= 0) {
				prec++;
			}

			if (flags & LONGDBL) {
				fparg.ldbl = va_arg(ap, long double);
				if (fparg.ldbl < 0) {
					sign = '-';
				}
			} else {
				fparg.dbl = va_arg(ap, double);
				if (fparg.dbl < 0) {
					sign = '-';
				}
			}

			flags |= FPT;
			dtoaresult = cvt(&fparg, xdigs, prec, flags, &softsign, &expt, ch,
					&ndig);
			if (dtoaresult == NULL) {
				errno = ENOMEM;
				ret = WEOF;
				goto error;
			}
			if (prec < 0) {
				prec = ndig;
			}
			if (expt == INT_MAX) {
				ox[1] = '\0';
			}
			goto fp_common;
		case 'e': /* anomalous precision */
		case 'E':
			prec = (prec == -1) ? DEFPREC + 1 : prec + 1;
			/* FALLTHROUGH */
			goto fp_begin;
		case 'f': /* always print trailing zeroes */
		case 'F':
			if (prec != 0) {
				flags |= ALT;
			}
		case 'g':
		case 'G':
			if (prec == 0) {
				prec = 1;
			}
			if (prec == -1) {
				prec = DEFPREC;
			}

fp_begin:
			if (flags & LONGDBL) {
				fparg.ldbl = va_arg(ap, long double);
				if (fparg.ldbl < 0) {
					sign = '-';
				}
			} else {
				fparg.dbl = va_arg(ap, double);
				if (fparg.dbl < 0) {
					sign = '-';
				}
			}

			flags |= FPT;
			dtoaresult = cvt(&fparg, xdigs, prec, flags, &softsign, &expt, ch,
					&ndig);
			if (dtoaresult == NULL) {
				errno = ENOMEM;
				ret = WEOF;
				goto error;
			}
			if (prec < 0) {
				prec = ndig;
			}

fp_common:
			if (convbuf != NULL) {
				free(convbuf);
			}
			cp = convbuf = __mbsconv(dtoaresult, -1, locale);
			if (cp == NULL) {
				ret = WEOF;
				goto error;
			}
			__freedtoa(dtoaresult);
			if (expt == INT_MAX) {
				if (*cp == 'N') {
					cp = (ch >= 'a') ? L"nan" : L"NAN";
					sign = '\0';
				} else {
					cp = (ch >= 'a') ? L"inf" : L"INF";
				}
				size = 3;
				break;
			}
			if (ch == 'g' || ch == 'G') {
				if (expt <= -4 || expt > prec) {
					ch = (ch == 'g') ? 'e' : 'E';
				} else {
					ch = 'g';
				}
			}
			if (ch <= 'e') { /* 'e' or 'E' fmt */
				--expt;
				expsize = exponent(expstr, expt, ch);
				size = expsize + ndig;
				if (ndig > 1 || (flags & ALT)) {
					++size;
				}
			} else if (ch == 'f') { /* f fmt */
				if (expt > 0) {
					size = expt;
					if (prec || (flags & ALT)) {
						size += prec + 1;
					}
				} else {
					/* "0.X" */
					size = prec + 2;
				}
			} else if (expt >= ndig) { /* fixed g fmt */
				size = expt;
				if (flags & ALT) {
					++size;
				}
			} else {
				size = ndig + (expt > 0 ? 1 : 2 - expt);
			}
			if (grouping && expt > 0) {
				/* space for thousands' grouping */
				nseps = nrepeats = 0;
				size += grouping_init(expt, nseps, nrepeats, grouping);
			}
			if (softsign) {
				sign = '-';
			}
			break;
#endif /* FLOATING_POINT */
		case 'n':
			if (flags & MAXINT)
				*va_arg(ap, intmax_t *) = ret;
			else if (flags & PTRDIFFT)
				*va_arg(ap, ptrdiff_t *) = ret;
			else if (flags & SIZEINT)
				*va_arg(ap, ssize_t *) = ret;	/* XXX */
			else if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else if (flags & CHARINT)
				*va_arg(ap, signed char *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			if (flags & INTMAX_SIZE) {
				ujval = UJARG();
			} else {
				ulval = UARG();
			}
			base = 8;
			goto nosign;
		case 'p':
			/*
			 * ``The argument shall be a pointer to void.  The
			 * value of the pointer is converted to a sequence
			 * of printable characters, in an implementation-
			 * defined manner.''
			 *	-- ANSI X3J11
			 */
			ujval = (uintmax_t)(uintptr_t)va_arg(ap, void *);
			base = 16;
			xdigs = xdigs_lower;
			flags = (flags & ~QUADINT) | HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 'S':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 's':
			if ((flags & LONGINT) != 0) {
				cp = va_arg(ap, wchar_t*);
				if (cp == NULL)
					cp = L"(null)";
			} else {
				char *mc;

				if (convbuf != NULL)
					free(convbuf);
				mc = va_arg(ap, char *);
				if (mc == NULL)
					cp = L"(null)";
				else {
					convbuf = __mbsconv(mc, prec, locale);
					if (convbuf == NULL) {
						fp->_flags |= __SERR;
						goto error;
					}
					cp = convbuf;
				}
			}

			if (prec >= 0) {
				/*
				 * can't use STRLEN; can only look for the
				 * NUL in the first `prec' characters, and
				 * STRLEN() will go further.
				 */
				const wchar_t *p = wmemchr(cp, 0, (size_t)prec);

				if (p != NULL) {
					size = (int)(p - cp);
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else {
				size_t rlen = wcslen(cp);
				if (rlen > INT_MAX) {
					errno = EOVERFLOW;
					ret = WEOF;
					goto error;
				}
				size = (int)rlen;
			}
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			if (flags & INTMAX_SIZE) {
				ujval = UJARG();
			} else {
				ulval = UARG();
			}
			base = 10;
			goto nosign;
		case 'X':
			xdigs = xdigs_upper;
			goto hex;
		case 'x':
			xdigs = xdigs_lower;
hex:
			if (flags & INTMAX_SIZE)
				ujval = UJARG();
			else
				ulval = UARG();
			base = 16;
			/* leading 0x/X only if non-zero */
			if ((flags & ALT)
					&& (flags & INTMAX_SIZE ? ujval != 0 : ulval != 0)) {
				flags |= HEXPREFIX;
				ox[1] = ch;
			}

			flags &= ~GROUPING;
			/* unsigned conversions */
nosign:
			sign = '\0';
			/*-
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:
			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*-
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 *
			 * ``The C Standard is clear enough as is.  The call
			 * printf("%#.0o", 0) should print 0.''
			 *	-- Defect Report #151
			 */
			bp = cp = buf + BUF;
			if (flags & INTMAX_SIZE) {
				if (ujval != 0 || prec != 0 || ((flags & ALT) && base == 8)) {
					bp = __ujtoa(ujval, cp, base, flags & ALT, xdigs,
							flags & GROUPING, thousands_sep, grouping);
				}
			} else {
				if (ulval != 0 || prec != 0 || ((flags & ALT) && base == 8)) {
					bp = __ultoa(ulval, cp, base, flags & ALT, xdigs,
							flags & GROUPING, thousands_sep, grouping);
				}
			}
			cp = bp;
			size = (int)(buf + BUF - bp);
			if (size > BUF) /* should never happen */
				abort();
			break;
		default: /* "%?" prints ?, unless ? is NUL */
			if (ch == '\0')
				goto done;
			/* pretend it was %c with argument ch */
			*buf = ch;
			cp = buf;
			size = 1;
			sign = '\0';
			break;
		}

		/*
		 * All reasonable formats wind up here.  At this point, `cp'
		 * points to a string which (if not flags&LADJUST) should be
		 * padded out to `width' places.  If flags&ZEROPAD, it should
		 * first be prefixed by any sign or other prefix; otherwise,
		 * it should be blank padded before the prefix is emitted.
		 * After any left-hand padding and prefixing, emit zeroes
		 * required by a decimal [diouxX] precision, then print the
		 * string proper, then emit zeroes required by any leftover
		 * floating precision; finally, if LADJUST, pad with blanks.
		 *
		 * Compute actual size, so we know how much to pad.
		 * fieldsz excludes decimal prec; realsz includes it.
		 */
		fieldsz = size;
		if (sign)
			fieldsz++;
		else if (ISHEX(flags))
			fieldsz += 2;

		realsz = dprec > size ? dprec : size;
		if (sign)
			realsz++;
		else if (ISHEX(flags))
			realsz += 2;

		prsize = width > realsz ? width : realsz;
		if ((unsigned)ret + prsize > INT_MAX) {
			ret = WEOF;
			goto error;
		}

		/* right-adjusting blank padding */
		if ((flags & (LADJUST | ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (ISHEX(flags)) {
			ox[0] = '0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - size, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else { /* glue together f_p fragments */
			if (ch >= 'f') { /* 'f' or 'g' */
#ifndef OLD_PROPER
				if (expt <= 0) {
					PRINT(zeroes, 1);
					if (prec || (flags & ALT)) {
						PRINT(decimal_point, 1);
					}
					PAD(-expt, zeroes);
					PRINT(cp, ndig);
					prec += expt;
				} else {
					PRINTANDPAD(cp, convbuf + ndig, expt, zeroes);
					if (grouping) {
						while (nseps > 0 || nrepeats > 0) {
							if (nrepeats > 0)
								nrepeats--;
							else {
								grouping--;
								nseps--;
							}
							PRINT(&thousands_sep, 1);
							PRINTANDPAD(cp, convbuf + ndig, *grouping, zeroes);
							cp += *grouping;
						}
						if (cp > convbuf + ndig)
							cp = convbuf + ndig;
					}
					if (prec || (flags & ALT)) {
						buf[0] = *decimal_point;
						PRINT(buf, 1);
					}
				}
				PRINTANDPAD(cp, convbuf + ndig, prec, zeroes);
			} else { /* %[eE] or sufficiently long %[gG] */
				if (prec > 1 || (flags & ALT)) {
					buf[0] = *cp++;
					buf[1] = *decimal_point;
					PRINT(buf, 2);
					PRINT(cp, ndig - 1);
					PAD(prec - ndig, zeroes);
				} else

#else /* OLD_PROPER */
				if ((fparg.dbl == 0) || ((fparg.ldbl == 0) && (flags & LONGDBL))) {
					PRINT(L"0", 1);
					if (expt < ndig || (flags & ALT) != 0) {
						PRINT(decimal_point, 1);
						PAD(ndig - 1, zeroes);
					}
				} else if (expt <= 0) {
					PRINT(L"0", 1);
					if (prec || (flags & ALT)) {
						PRINT(decimal_point, 1);
					}
					PAD(-expt, zeroes);
					PRINT(cp, ndig);
					prec += expt;
				} else if (expt >= ndig) {
					PRINT(cp, ndig);
					PAD(expt - ndig, zeroes);
					if (flags & ALT)
						PRINT(L".", 1);
				} else {
					PRINT(cp, expt);
					cp += expt;
					PRINT(L".", 1);
					PRINT(cp, ndig - expt);
				}
			} else { /* 'e' or 'E' */
				if (ndig > 1 || (flags & ALT)) {
					ox[0] = *cp++;
					ox[1] = '.';
					PRINT(ox, 2);
					if (fparg.dbl || (fparg.ldbl && (flags & LONGDBL)) || (flags & ALT) == 0) {
						PRINT(cp, ndig - 1);
					} else
						/* 0.[0..] */
						/* __dtoa irregularity */
						PAD(ndig - 1, zeroes);
				} else
#endif  /* OLD_PROPER */
					/* XeYYY */
					PRINT(cp, 1);
				PRINT(expstr, expsize);
			}
		}
#else /* FLOATING_POINT */
		PRINT(cp, size);
#endif /* FLOATING_POINT */

		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		if (width > INT_MAX - ret) {
			errno = EOVERFLOW;
			ret = WEOF;
			goto error;
		}
		ret += width > realsz ? width : realsz;

		FLUSH(); /* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
	if (convbuf != NULL)
		free(convbuf);
	if (__sferror(fp))
		ret = WEOF;
	return ret;
	/* NOTREACHED */
}
