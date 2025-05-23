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

#include <sys/cdefs.h>
#if defined(LIBC_SCCS) && !defined(lint)
#if 0
static char sccsid[] = "@(#)vfprintf.c	5.2 (Berkeley) 6/27/88";
static char sccsid[] = "@(#)vfprintf.c	8.1 (Berkeley) 6/4/93";
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

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

#include "reentrant.h"
#include "setlocale.h"
#include "local.h"
#include "fvwrite.h"
#include "extern.h"

static int __sprint(FILE *, struct __suio *);
static int __sbprintf(FILE *, const char *, va_list);
static char *__ultoa(u_long, const char *, int, int, const char *);
static char *__uqtoa(u_quad_t, const char *, int, int, const char *);

static const char xdigs_lower[16] = "0123456789abcdef";
static const char xdigs_upper[16] = "0123456789ABCDEF";

/*
 * Flush out all the vectors defined by the given uio,
 * then reset it so that it can be reused.
 */
static int
__sprint(FILE *fp, struct __suio *uio)
{
	int err;

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(uio != NULL);

	if (uio->uio_resid == 0) {
		uio->uio_iovcnt = 0;
		return (0);
	}
	err = __sfvwrite(fp, uio);
	uio->uio_resid = 0;
	uio->uio_iovcnt = 0;
	return (err);
}

/*
 * Helper function for `fprintf to unbuffered unix file': creates a
 * temporary buffer.  We only work on write-only files; this avoids
 * worries about ungetc buffers and so forth.
 */
static int
__sbprintf(FILE *fp, const char *fmt, va_list ap)
{
	int ret;
	FILE fake;
    struct __sfileext fakeext;
	unsigned char buf[BUFSIZ];

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(fmt != NULL);

    _FILEEXT_SETUP(&fake, &fakeext);

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
	ret = vfprintf(&fake, fmt, ap);
	if (ret >= 0 && fflush(&fake))
		ret = EOF;
	if (fake._flags & __SERR)
		fp->_flags |= __SERR;
	return (ret);
}

/*
 * Macros for converting digits to letters and vice versa
 */
#define	to_digit(c)	((c) - '0')
#define is_digit(c)	((unsigned)to_digit(c) <= 9)
#define	to_char(n)	((n) + '0')

/*
 * Convert an unsigned long to ASCII for printf purposes, returning
 * a pointer to the first character of the string representation.
 * Octal numbers can be forced to have a leading zero; hex numbers
 * use the given digits.
 */
static char *
__ultoa(u_long val, const char *endp, int base, int octzero, const char *xdigs)
{
	char *cp = __UNCONST(endp);
	long sval;

	/*
	 * Handle the three cases separately, in the hope of getting
	 * better/faster code.
	 */
	switch (base) {
	case 10:
		if (val < 10) { /* many numbers are 1 digit */
			*--cp = to_char(val);
			return (cp);
		}
		/*
		 * On many machines, unsigned arithmetic is harder than
		 * signed arithmetic, so we do at most one unsigned mod and
		 * divide; this is sufficient to reduce the range of
		 * the incoming value to where signed arithmetic works.
		 */
		if (val > LONG_MAX) {
			*--cp = to_char(val % 10);
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default: /* oops */
		abort();
	}
	return (cp);
}

/* Identical to __ultoa, but for quads. */
static char *
__uqtoa(u_quad_t val, const char *endp, int base, int octzero, const char *xdigs)
{
	char *cp = __UNCONST(endp);
	quad_t sval;

	/* quick test for small values; __ultoa is typically much faster */
	/* (perhaps instead we should run until small, then call __ultoa?) */
	if (val <= ULONG_MAX)
		return (__ultoa((u_long) val, endp, base, octzero, xdigs));
	switch (base) {
	case 10:
		if (val < 10) {
			*--cp = to_char(val % 10);
			return (cp);
		}
		if (val > QUAD_MAX) {
			*--cp = to_char(val % 10);
			sval = val / 10;
		} else
			sval = val;
		do {
			*--cp = to_char(sval % 10);
			sval /= 10;
		} while (sval != 0);
		break;

	case 8:
		do {
			*--cp = to_char(val & 7);
			val >>= 3;
		} while (val);
		if (octzero && *cp != '0')
			*--cp = '0';
		break;

	case 16:
		do {
			*--cp = xdigs[val & 15];
			val >>= 4;
		} while (val);
		break;

	default:
		abort();
	}
	return (cp);
}

#ifdef FLOATING_POINT
#include <locale.h>
#include <math.h>
#include "floatio.h"

#define	BUF		(MAXEXP+MAXFRACT+1)	/* + decimal point */
#define	DEFPREC		6

static char *cvt(double, int, int, char *, int *, int, int *);
static int exponent(char *, int, int);

#else /* no FLOATING_POINT */

#define	BUF		100

#endif /* FLOATING_POINT */

/*
 * Flags used during conversion.
 */
#define	ALT		0x001		/* alternate form */
#define	HEXPREFIX	0x002		/* add 0x or 0X prefix */
#define	LADJUST		0x004		/* left adjustment */
#define	LONGDBL		0x008		/* long double; unimplemented */
#define	LONGINT		0x010		/* long integer */
#define	QUADINT		0x020		/* quad integer */
#define	SHORTINT	0x040		/* short integer */
#define	MAXINT		0x080		/* (unsigned) intmax_t */
#define	PTRINT		0x100		/* (unsigned) ptrdiff_t */
#define	SIZEINT		0x200		/* (signed) size_t */
#define	ZEROPAD		0x400		/* zero (as opposed to blank) pad */
#define FPT		0x800		/* Floating point number */
/*
int vfprintf_unlocked(FILE *, const char *, va_list);

int
vfprintf(fp, fmt0, ap)
	FILE *fp;
	const char *fmt0;
	va_list ap;
 {
    int ret;

    FLOCKFILE(fp);
    ret = vfprintf_unlocked(fp, fmt0, ap);
    FUNLOCKFILE(fp);
	return (ret);
 }
*/

int
vfprintf(FILE *fp, const char *fmt0, va_list ap)
{
	return (vfprintf_l(fp, __get_locale(), fmt0, ap));
}

int
vfprintf_l(FILE *fp, locale_t locale, const char *fmt0, va_list ap)
{
	const char *fmt; 		/* format string */
	int ch; 			/* character from fmt */
	int n, m; 			/* handy integers (short term usage) */
	const char *cp; 		/* handy char pointer (short term usage) */
	char *bp; 			/* handy char pointer (short term usage) */
	int flags; 			/* flags as above */
	int ret; 			/* return value accumulator */
	int width; 			/* width from format (%8d), or 0 */
	int prec; 			/* precision from format (%.3d), or -1 */
	char sign; 			/* sign prefix (' ', '+', '-', or \0) */
#ifdef FLOATING_POINT
    	struct lconv *lc;
	char *decimal_point;
	char softsign;			/* temporary negative sign for floats */
	double _double;			/* double precision arguments %[eEfgG] */
	int expt;			/* integer value of exponent */
	int expsize;			/* character count for expstr */
	int ndig;			/* actual number of digits returned by cvt */
	char expstr[7];			/* buffer for exponent string */
#endif
	u_long	ulval;			/* integer arguments %[diouxX] */
	u_quad_t uqval;			/* %q integers */
	int base; 			/* base for [diouxX] conversion */
	int dprec;			/* a copy of prec if [diouxX], 0 otherwise */
	int fieldsz;			/* field size expanded by sign, etc */
	int realsz;			/* field size expanded by dprec */
	int size;			/* size of converted field or string */
	const char *xdigs;		/* digits for [xX] conversion */
#define NIOV 8
	struct __suio uio;		/* output information: summary */
	struct __siov iov[NIOV];/* ... and individual io vectors */
	struct __siov *iovp;	/* for PRINT macro */
	char buf[BUF];			/* space for %c, %[diouxX], %[eEfgG] */
	char ox[2];			/* space for 0x hex-prefix */
	wchar_t wc;
	mbstate_t ps;
//	intmax_t _intmax;
//	uintmax_t _uintmax;

	/*
	 * Choose PADSIZE to trade efficiency vs. size.  If larger printf
	 * fields occur frequently, increase PADSIZE and make the initialisers
	 * below longer.
	 */
#define	PADSIZE	16		/* pad chunk size */
	static const char blanks[PADSIZE] =
	 {' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '};

	static const char zeroes[PADSIZE] =
	{'0','0','0','0','0','0','0','0','0','0','0','0','0','0','0','0'};


	/*
	 * BEWARE, these `goto error' on error, and PAD uses `n'.
	 */
#define	PRINT(ptr, len) { 					\
	iovp->iov_base = __UNCONST(ptr); 				\
	iovp->iov_len = (len); 					\
	uio.uio_resid += (len); 				\
	iovp++; 						\
	if (++uio.uio_iovcnt >= NIOV) { 			\
		if (__sprint(fp, &uio)) 			\
			goto error; 				\
		iovp = iov; 					\
	} 							\
}
#define	PAD(howmany, with) { 					\
	if ((n = (howmany)) > 0) { 				\
		while (n > PADSIZE) { 				\
			PRINT(with, PADSIZE); 			\
			n -= PADSIZE; 				\
		} 						\
		PRINT(with, n); 				\
	} 							\
}
#define	FLUSH() { 						\
	if (uio.uio_resid && __sprint(fp, &uio)) 		\
		goto error; 					\
	uio.uio_iovcnt = 0; 					\
	iovp = iov; 						\
}

	/*
	 * To extend shorts properly, we need both signed and unsigned
	 * argument extraction methods.
	 */
#define	SARG() 							\
	(flags&MAXINT ? va_arg(ap, intmax_t) : 			\
	    flags&PTRINT ? va_arg(ap, ptrdiff_t) : 		\
	    flags&SIZEINT ? va_arg(ap, ssize_t) : /* XXX */ 	\
	    flags&QUADINT ? va_arg(ap, quad_t) : 		\
	    flags&LONGINT ? va_arg(ap, long) : 			\
	    flags&SHORTINT ? (long)(short)va_arg(ap, int) : 	\
	    (long)va_arg(ap, int))
#define	UARG() 							\
	(flags&MAXINT ? va_arg(ap, uintmax_t) : 		\
	    flags&PTRINT ? va_arg(ap, uintptr_t) : /* XXX */ 	\
	    flags&SIZEINT ? va_arg(ap, size_t) : 		\
	    flags&QUADINT ? va_arg(ap, u_quad_t) : 		\
	    flags&LONGINT ? va_arg(ap, u_long) : 		\
	    flags&SHORTINT ? (u_long)(u_short)va_arg(ap, int) : \
	    (u_long)va_arg(ap, u_int))

    _SET_ORIENTATION(fp, -1);

	/* sorry, fprintf(read_only_file, "") returns EOF, not 0 */
	if (cantwrite(fp)) {
		return (EOF);
	}

	/* optimise fprintf(stderr) (and other unbuffered Unix files) */
	if ((fp->_flags & (__SNBF|__SWR|__SRW)) == (__SNBF|__SWR) && fp->_file >= 0) {
        ret = __sbprintf(fp, fmt0, ap);
		return (ret);
	}

	fmt = fmt0;
	uio.uio_iov = iovp = iov;
	uio.uio_resid = 0;
	uio.uio_iovcnt = 0;
	ret = 0;
    ulval = 0;
    uqval = 0;
    xdigs = NULL;

#ifdef FLOATING_POINT
    lc = localeconv_l(locale);
	decimal_point = lc->decimal_point;
#endif

	memset(&ps, 0, sizeof(ps));

	/*
	 * Scan the format for conversions (`%' character).
	 */
	for (;;) {
		cp = fmt;
		n = mbrtowc(&wc, fmt, MB_CUR_MAX, &ps);
		while (n > 0) {
			fmt += n;
			if (wc == '%') {
				fmt--;
				break;
			}
		}
		if ((m = fmt - cp) != 0) {
			PRINT(cp, m);
			ret += m;
		}
		if (n <= 0)
			goto done;
		fmt++;		/* skip over '%' */

		flags = 0;
		dprec = 0;
		width = 0;
		prec = -1;
		sign = '\0';
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
			if ((width = va_arg(ap, int)) >= 0)
				goto rflag;
			width = -width;
			/* FALLTHROUGH */
		case '-':
			flags |= LADJUST;
			goto rflag;
		case '+':
			sign = '+';
			goto rflag;
		case '.':
			if ((ch = *fmt++) == '*') {
				n = va_arg(ap, int);
				prec = n < 0 ? -1 : n;
				goto rflag;
			}
			n = 0;
			while (is_digit(ch)) {
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			}
			prec = n < 0 ? -1 : n;
			goto reswitch;
		case '0':
			/*
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
				n = 10 * n + to_digit(ch);
				ch = *fmt++;
			} while (is_digit(ch));
			width = n;
			goto reswitch;
#ifdef FLOATING_POINT
		case 'L':
			flags |= LONGDBL;
			goto rflag;
#endif
		case 'h':
			flags |= SHORTINT;
			goto rflag;
		case 'j':
			flags |= MAXINT;
			goto rflag;
		case 'l':
			if (*fmt == 'l') {
				fmt++;
				flags |= QUADINT;
			} else {
				flags |= LONGINT;
			}
			goto rflag;
		case 'q':
			flags |= QUADINT;
			goto rflag;
		case 't':
			flags |= PTRINT;
			goto rflag;
		case 'z':
			flags |= SIZEINT;
			goto rflag;
		case 'c':
			*buf = va_arg(ap, int);
            cp = buf;
			size = 1;
			sign = '\0';
			break;
		case 'D':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'd':
		case 'i':
			if (flags & QUADINT) {
				uqval = va_arg(ap, quad_t);
				if ((quad_t) uqval < 0) {
					uqval = -uqval;
					sign = '-';
				}
			} else {
				ulval = SARG();
				if ((long) ulval < 0) {
					ulval = -ulval;
					sign = '-';
				}
			}
			base = 10;
			goto number;
#ifdef FLOATING_POINT
		case 'e': /* anomalous precision */
		case 'E':
			prec = (prec == -1) ? DEFPREC + 1 : prec + 1;
			/* FALLTHROUGH */
			goto fp_begin;
		case 'f': /* always print trailing zeroes */
			if (prec != 0) {
				flags |= ALT;
			}
		case 'g':
		case 'G':
			if (prec == -1)
				prec = DEFPREC;
fp_begin:
			_double = va_arg(ap, double);
			/* do this before tricky precision changes */
			if (isinf(_double)) {
				if (_double < 0)
					sign = '-';
				cp = "Inf";
				size = 3;
				break;
			}
			if (isnan(_double)) {
				cp = "NaN";
				size = 3;
				break;
			}
			flags |= FPT;
			cp = cvt(_double, prec, flags, &softsign, &expt, ch, &ndig);
			if (ch == 'g' || ch == 'G') {
				if (expt <= -4 || expt > prec)
					ch = (ch == 'g') ? 'e' : 'E';
				else
					ch = 'g';
			}
			if (ch <= 'e') { /* 'e' or 'E' fmt */
				--expt;
				expsize = exponent(expstr, expt, ch);
				size = expsize + ndig;
				if (ndig > 1 || flags & ALT)
					++size;
			} else if (ch == 'f') { /* f fmt */
				if (expt > 0) {
					size = expt;
					if (prec || flags & ALT)
						size += prec + 1;
				} else
					/* "0.X" */
					size = prec + 2;
			} else if (expt >= ndig) { /* fixed g fmt */
				size = expt;
				if (flags & ALT)
					++size;
			} else
				size = ndig + (expt > 0 ? 1 : 2 - expt);

			if (softsign)
				sign = '-';
			break;
#endif /* FLOATING_POINT */
		case 'n':
			if (flags & MAXINT)
				*va_arg(ap, intmax_t *) = ret;
			else if (flags & PTRINT)
				*va_arg(ap, ptrdiff_t *) = ret;
			else if (flags & SIZEINT)
				*va_arg(ap, ssize_t *) = ret;	/* XXX */
			else if (flags & QUADINT)
				*va_arg(ap, quad_t *) = ret;
			else if (flags & LONGINT)
				*va_arg(ap, long *) = ret;
			else if (flags & SHORTINT)
				*va_arg(ap, short *) = ret;
			else
				*va_arg(ap, int *) = ret;
			continue;	/* no output */
		case 'O':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'o':
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
				ulval = UARG();
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
			ulval = (u_long) va_arg(ap, void*);
			base = 16;
			xdigs = xdigs_lower;
			flags = (flags & ~QUADINT) | HEXPREFIX;
			ch = 'x';
			goto nosign;
		case 's':
			if ((cp = va_arg(ap, char*)) == NULL)
				cp = "(null)";
			if (prec >= 0) {
				/*
				 * can't use strlen; can only look for the
				 * NUL in the first `prec' characters, and
				 * strlen() will go further.
				 */
				char *p = memchr(cp, 0, prec);

				if (p != NULL) {
					size = p - cp;
					if (size > prec)
						size = prec;
				} else
					size = prec;
			} else
				size = strlen(cp);
			sign = '\0';
			break;
		case 'U':
			flags |= LONGINT;
			/*FALLTHROUGH*/
		case 'u':
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
				ulval = UARG();
			base = 10;
			goto nosign;
		case 'X':
			xdigs = xdigs_upper;
			goto hex;
		case 'x':
			xdigs = xdigs_lower;
hex:
			if (flags & QUADINT)
				uqval = va_arg(ap, u_quad_t);
			else
				ulval = UARG();
			base = 16;
			/* leading 0x/X only if non-zero */
			if ((flags & ALT) && (flags & QUADINT ? uqval != 0 : ulval != 0))
				flags |= HEXPREFIX;

			/* unsigned conversions */
nosign:
			sign = '\0';
			/*
			 * ``... diouXx conversions ... if a precision is
			 * specified, the 0 flag will be ignored.''
			 *	-- ANSI X3J11
			 */
number:
			if ((dprec = prec) >= 0)
				flags &= ~ZEROPAD;

			/*
			 * ``The result of converting a zero value with an
			 * explicit precision of zero is no characters.''
			 *	-- ANSI X3J11
			 */
			bp = buf + BUF;
			if (flags & QUADINT) {
				if (uqval != 0 || prec != 0) {
					bp = __uqtoa(uqval, cp, base, flags & ALT, xdigs);
				}
			} else {
				if (ulval != 0 || prec != 0) {
					bp = __ultoa(ulval, cp, base, flags & ALT, xdigs);
				}
			}
            cp = bp;
			size = buf + BUF - bp;
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
		else if (flags & HEXPREFIX)
			fieldsz += 2;
		realsz = dprec > fieldsz ? dprec : fieldsz;

		/* right-adjusting blank padding */
		if ((flags & (LADJUST | ZEROPAD)) == 0)
			PAD(width - realsz, blanks);

		/* prefix */
		if (sign) {
			PRINT(&sign, 1);
		} else if (flags & HEXPREFIX) {
			ox[0] = '0';
			ox[1] = ch;
			PRINT(ox, 2);
		}

		/* right-adjusting zero padding */
		if ((flags & (LADJUST | ZEROPAD)) == ZEROPAD)
			PAD(width - realsz, zeroes);

		/* leading zeroes from decimal precision */
		PAD(dprec - fieldsz, zeroes);

		/* the string or number proper */
#ifdef FLOATING_POINT
		if ((flags & FPT) == 0) {
			PRINT(cp, size);
		} else { /* glue together f_p fragments */
			if (ch >= 'f') { /* 'f' or 'g' */
				if (_double == 0) {
					/* kludge for __dtoa irregularity */
					PRINT("0", 1);
					if (expt < ndig || (flags & ALT) != 0) {
						PRINT(decimal_point, 1);
						PAD(ndig - 1, zeroes);
					}
				} else if (expt <= 0) {
					PRINT("0", 1);
					PRINT(decimal_point, 1);
					PAD(-expt, zeroes);
					PRINT(cp, ndig);
				} else if (expt >= ndig) {
					PRINT(cp, ndig);
					PAD(expt - ndig, zeroes);
					if (flags & ALT)
						PRINT(".", 1);
				} else {
					PRINT(cp, expt);
					cp += expt;
					PRINT(".", 1);
					PRINT(cp, ndig - expt);
				}
			} else { /* 'e' or 'E' */
				if (ndig > 1 || (flags & ALT)) {
					ox[0] = *cp++;
					ox[1] = '.';
					PRINT(ox, 2);
					if (_double || (flags & ALT) == 0) {
						PRINT(cp, ndig - 1);
					} else
						/* 0.[0..] */
						/* __dtoa irregularity */
						PAD(ndig - 1, zeroes);
				} else
					/* XeYYY */
					PRINT(cp, 1);
				PRINT(expstr, expsize);
			}
		}
#else
		PRINT(cp, size);
#endif
		/* left-adjusting padding (always blank) */
		if (flags & LADJUST)
			PAD(width - realsz, blanks);

		/* finally, adjust ret */
		ret += width > realsz ? width : realsz;

		FLUSH(); /* copy out the I/O vectors */
	}
done:
	FLUSH();
error:
	if (__sferror(fp)) {
		ret = EOF;
	}
	return (ret);
}

#ifdef FLOATING_POINT

static char *
cvt(double value, int ndigits, int flags, char *sign, int *decpt, int ch, int *length)
{
	int mode, dsgn;
	char *digits, *bp, *rve;

	_DIAGASSERT(decpt != NULL);
	_DIAGASSERT(length != NULL);
	_DIAGASSERT(sign != NULL);

	if (ch == 'f') {
		mode = 3; /* ndigits after the decimal point */
	} else {
		/* To obtain ndigits after the decimal point for the 'e'
		 * and 'E' formats, round to ndigits + 1 significant
		 * figures.
		 */
		if (ch == 'e' || ch == 'E') {
			ndigits++;
		}
		mode = 2; /* ndigits significant digits */
	}

	digits = __dtoa(value, mode, ndigits, decpt, &dsgn, &rve);
	if (dsgn) {
		value = -value;
		*sign = '-';
	} else
		*sign = '\000';
	if ((ch != 'g' && ch != 'G') || (flags & ALT)) { /* Print trailing zeros */
		bp = digits + ndigits;
		if (ch == 'f') {
			if (*digits == '0' && value)
				*decpt = -ndigits + 1;
			bp += *decpt;
		}
		if (value == 0) /* kludge for __dtoa irregularity */
			rve = bp;
		while (rve < bp)
			*rve++ = '0';
	}
	*length = rve - digits;
	return (digits);
}

static int
exponent(char *p0, int exp, int fmtch)
{
	char *p, *t;
	char expbuf[MAXEXP];

	_DIAGASSERT(p0 != NULL);

	p = p0;
	*p++ = fmtch;
	if (exp < 0) {
		exp = -exp;
		*p++ = '-';
	} else
		*p++ = '+';
	t = expbuf + MAXEXP;
	if (exp > 9) {
		do {
			*--t = to_char(exp % 10);
		} while ((exp /= 10) > 9);
		*--t = to_char(exp);
		for (; t < expbuf + MAXEXP; *p++ = *t++)
			;
	} else {
		*p++ = '0';
		*p++ = to_char(exp);
	}
	return (p - p0);
}
#endif /* FLOATING_POINT */
