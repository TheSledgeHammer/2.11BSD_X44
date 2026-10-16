/*	$NetBSD: vfwscanf.c,v 1.13 2022/04/19 20:32:16 rillig Exp $	*/

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
static char sccsid[] = "@(#)vfscanf.c	8.1 (Berkeley) 6/4/93";
__FBSDID("$FreeBSD: src/lib/libc/stdio/vfwscanf.c,v 1.12 2004/05/02 20:13:29 obrien Exp $");
__RCSID("$NetBSD: vfwscanf.c,v 1.13 2022/04/19 20:32:16 rillig Exp $");
#endif
#endif /* LIBC_SCCS and not lint */

#include "namespace.h"

#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "reentrant.h"
#include "local.h"
#include "xscan.h"

#ifdef FLOATING_POINT
#include "floatio.h"
#endif

#ifdef FLOATING_POINT
static int parsefloat(FILE *, wchar_t *, wchar_t *, locale_t);
#endif
static const wchar_t *swcl(wchar_t *, const wchar_t *, int *, locale_t);

int vfwscanf(FILE *, const wchar_t *, va_list);
int vfwscanf_l(FILE *, locale_t, const wchar_t *, va_list);
int vfwscanf_l_unlocked(FILE *, locale_t, const wchar_t *, va_list);

static void
ccltable(wchar_t *tab, const wchar_t **start, const wchar_t **end, locale_t locale)
{
	int i, len;

	for (i = 0; i < CCLTABMAXLEN; i++) {
		if (tab[wctob_l(i, locale)]) {
			len++;
		}
	}
	*start = &tab[wctob_l(0, locale)];
	*end = &tab[wctob_l(len-1, locale)];
}

static int
inccl(int compl, wint_t wi, wchar_t *tab, locale_t locale)
{
	const wchar_t *start, *end;

	ccltable(tab, &start, &end, locale);
	if (compl) {
		return (wmemchr(start, wi, (size_t)(end - start)) == NULL);
	} else {
		return (wmemchr(start, wi, (size_t)(end - start)) != NULL);
	}
}

int
vfwscanf(FILE *fp, const wchar_t *fmt0, va_list ap)
{
	return (vfwscanf_l(fp, __get_locale(), fmt0, ap));
}

int
vfwscanf_l(FILE *fp, locale_t locale, const wchar_t *fmt0, va_list ap)
{
	int ret;

	FLOCKFILE(fp);
	ret = vfwscanf_unlocked_l(fp, locale, fmt0, ap);
	FUNLOCKFILE(fp);
	return (ret);
}

#define SCANF_SKIP_SPACE() \
do { \
	wint_t tc; \
 \
	while ((tc = fgetmbrune(fp)) != WEOF && iswspace_l(tc, locale)) { \
		continue; \
	} \
	if (tc != WEOF) { \
		fungetmbrune(tc, fp); \
	} \
} while (0)

int
vfwscanf_l_unlocked(FILE *fp, locale_t locale, const wchar_t *fmt0, va_list ap)
{
    const wchar_t *fmt;
	wint_t c;		        /* character from format, or conversion */
	size_t width;	    	/* field width, or 0 */
	wchar_t *p;	        	/* points into all kinds of strings */
	int n;		        	/* handy integer */
	int flags;	        	/* flags as defined above */
	wchar_t *p0;	        /* saves original value of p when necessary */
	int nassigned;			/* number of fields assigned */
	int nconversions;		/* number of conversions */
	size_t nread;			/* number of characters consumed from fp */
	int base;				/* base argument to strtol/strtoul */
	wchar_t ccltab[CCLTABMAXLEN];/* character class table for %[...] */
	wchar_t buf[BUF];		/* buffer for numeric conversions */
	int cclcompl;			/* ccl is complemented? */
	wint_t wi;				/* handy wint_t */
	char *mbp;				/* multibyte string pointer for %c %s %[ */
	size_t nconv;			/* number of bytes in mb. conversion */
	static const mbstate_t initial;
	mbstate_t mbs;
	char mbbuf[MB_LEN_MAX];	/* temporary mb. character buffer */

	/* `basefix' is used to avoid `if' tests in the integer scanner */
	static short basefix[17] =
		{ 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(fmt0 != NULL);

	_SET_ORIENTATION(fp, 1);

	fmt = fmt0;
	nassigned = 0;
	nconversions = 0;
	nread = 0;
	base = 0;		/* XXX just to keep gcc happy */
	mbp = NULL;
	p = NULL;
	for (;;) {
		c = *fmt++;
		if (c == 0) {
			return (nassigned);
		}
		if (iswspace_l(c, locale)) {
			for (;;) {
				c = fgetmbrune(fp);
				if (c != WEOF && iswspace_l(c, locale)) {
					return (nassigned);
				}
				if (c != WEOF) {
					fungetmbrune(c, fp);
				}
				if (!iswspace_l(c, locale)) {
					break;
				}
				nread++;
			}
			continue;
		}
		if (c != '%') {
			goto literal;
		}
		width = 0;
		flags = 0;
		/*
		 * switch on the format.  continue if done;
		 * break once format type is derived.
		 */
again:
		c = *fmt++;
		switch (c) {
		case '%':
			SCANF_SKIP_SPACE();
literal:
			wi = fgetmbrune(fp);
			if (wi == WEOF) {
				goto input_failure;
			}
			if (wi != c) {
				fungetmbrune(wi, fp);
				goto match_failure;
			}
			nread++;
			continue;

		case '*':
			flags |= SUPPRESS;
			goto again;
		case 'j':
			flags |= INTMAXT;
			goto again;
		case 'l':
			if (flags & LONG) {
				flags &= ~LONG;
				flags |= LONGLONG;
			} else
				flags |= LONG;
			goto again;
		case 'q':
			flags |= LONGLONG; /* not quite */
			goto again;
		case 't':
			flags |= PTRDIFFT;
			goto again;
		case 'z':
			flags |= SIZET;
			goto again;
		case 'L':
			flags |= LONGDBL;
			goto again;
		case 'h':
			if (flags & SHORT) {
				flags &= ~SHORT;
				flags |= SHORTSHORT;
			} else
				flags |= SHORT;
			goto again;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			width = width * 10 + c - '0';
			goto again;

			/*
			 * Conversions.
			 * Those marked `compat' are for 4.[123]BSD compatibility.
			 *
			 * (According to ANSI, E and X formats are supposed
			 * to the same as e and x.  Sorry about that.)
			 */
		case 'D': /* compat */
			flags |= LONG;
			/* FALLTHROUGH */
		case 'd':
			c = CT_INT;
			base = 10;
			break;

		case 'i':
			c = CT_INT;
			base = 0;
			break;

		case 'O': /* compat */
			flags |= LONG;
			/* FALLTHROUGH */
		case 'o':
			c = CT_INT;
			base = 8;
			break;

		case 'u':
			c = CT_INT;
			base = 10;
			break;

		case 'X': /* compat   XXX */
			flags |= LONG;
			/* FALLTHROUGH */
		case 'x':
			flags |= PFXOK; /* enable 0x prefixing */
			c = CT_INT;
			base = 16;
			break;

#ifdef FLOATING_POINT
		case 'A':
		case 'E': /* compat   XXX */
		case 'F': /* compat */
		case 'G':
			flags |= LONG;
			/* FALLTHROUGH */
		case 'a':
		case 'e':
		case 'f':
		case 'g':
			c = CT_FLOAT;
			break;
#endif

		case 'S':
			flags |= LONG;
			/* FALLTHROUGH */
		case 's':
			c = CT_STRING;
			break;

		case '[':
			fmt = swcl(ccltab, fmt, &cclcompl, locale);
			flags |= NOSKIP;
			c = CT_CCL;
			break;

		case 'C':
			flags |= LONG;
			/* FALLTHROUGH */
		case 'c':
			flags |= NOSKIP;
			c = CT_CHAR;
			break;

		case 'p': /* pointer format is like hex */
			flags |= POINTER | PFXOK;
			c = CT_INT;
			base = 16;
			break;

		case 'n':
			nconversions++;
			if (flags & SUPPRESS) {	/* ??? */
				continue;
			}
			if (flags & SHORTSHORT) {
				*va_arg(ap, char *) = (char)nread;
			} else if (flags & SHORT) {
				*va_arg(ap, short *) = (short)nread;
			} else if (flags & LONG) {
				*va_arg(ap, long *) = nread;
			} else if (flags & LONGLONG) {
				*va_arg(ap, quad_t *) = nread;
			} else if (flags & INTMAXT) {
				*va_arg(ap, intmax_t *) = nread;
			} else if (flags & SIZET) {
				*va_arg(ap, size_t *) = nread;
			} else if (flags & PTRDIFFT) {
				*va_arg(ap, ptrdiff_t *) = nread;
			} else {
				*va_arg(ap, int *) = (int)nread;
			}
			continue;

		default:
			goto match_failure;

			/*
			 * Disgusting backwards compatibility hacks.	XXX
			 */
		case '\0': /* compat */
			return (EOF);
		}

		/*
		 * Consume leading white space, except for formats
		 * that suppress this.
		 */
		if ((flags & NOSKIP) == 0) {
			wi = fgetmbrune(fp);
			while (wi != WEOF && iswspace_l(wi, locale)) {
				nread++;
			}
			if (wi == WEOF) {
				goto input_failure;
			}
			fungetmbrune(wi, fp);
		}

		/*
		 * Do the conversion.
		 */
		switch (c) {

		case CT_CHAR:
			/* scan arbitrary characters (sets NOSKIP) */
			if (width == 0) {
				width = 1;
			}
			if (flags & LONG) {
				if (!(flags & SUPPRESS)) {
					p = va_arg(ap, wchar_t *);
				}
				n = 0;
				while (wi != WEOF && width-- != 0) {
					if (!(flags & SUPPRESS)) {
						*p++ = (wchar_t)wi;
					}
					n++;
				}
				if (n == 0) {
					goto input_failure;
				}
				nread += n;
				if (!(flags & SUPPRESS)) {
					nassigned++;
				}
			} else {
				if (!(flags & SUPPRESS)) {
					mbp = va_arg(ap, char *);
				}
				n = 0;
				mbs = initial;
				wi = fgetmbrune(fp);
				while (width != 0 && wi != WEOF ) {
					if (width >= MB_CUR_MAX && !(flags & SUPPRESS)) {
						nconv = wcrtomb_l(mbp, wi, &mbs, locale);
						if (nconv == (size_t)-1) {
							goto input_failure;
						}
					} else {
						nconv = wcrtomb_l(mbbuf, wi, &mbs, locale);
						if (nconv == (size_t)-1) {
							goto input_failure;
						}
						if (nconv > width) {
							fungetmbrune(wi, fp);
							break;
						}
						if (!(flags & SUPPRESS)) {
							memcpy(mbp, mbbuf, nconv);
						}
					}
					if (!(flags & SUPPRESS)) {
						mbp += nconv;
					}
					width -= nconv;
					n++;
				}
				if (n == 0) {
					goto input_failure;
				}
				nread += n;
				if (!(flags & SUPPRESS)) {
					nassigned++;
				}
			}
			nconversions++;
			break;

		case CT_CCL:
			/* scan a (nonempty) character class (sets NOSKIP) */
			if (width == 0) {
				width = ~0; /* `infinity' */
			}
			/* take only those things in the class */
			if ((flags & SUPPRESS) && (flags & LONG)) {
				n = 0;
				wi = fgetmbrune(fp);
				while (wi != WEOF && width-- != 0 && inccl(cclcompl, wi, ccltab, locale)) {
					n++;
				}
				if (wi != WEOF) {
					fungetmbrune(wi, fp);
				}
				if (n == 0) {
					goto match_failure;
				}
			} else if (flags & LONG) {
				p0 = p = va_arg(ap, wchar_t *);
				wi = fgetmbrune(fp);
				while (wi != WEOF && width-- != 0 && inccl(cclcompl, wi, ccltab, locale)) {
					*p++ = (wchar_t)wi;
				}
				if (wi != WEOF) {
					fungetmbrune(wi, fp);
				}
				n = (int)(p - p0);
				if (n == 0) {
					goto match_failure;
				}
				*p = 0;
				nassigned++;
			} else {
				if (!(flags & SUPPRESS)) {
					mbp = va_arg(ap, char *);
				}
				n = 0;
				mbs = initial;
				wi = fgetmbrune(fp);
				while (wi != WEOF && width != 0 && inccl(cclcompl, wi, ccltab, locale)) {
					if (width >= MB_CUR_MAX && !(flags & SUPPRESS)) {
						nconv = wcrtomb_l(mbp, wi, &mbs, locale);
						if (nconv == (size_t)-1) {
							goto input_failure;
						}
					} else {
						nconv = wcrtomb_l(mbbuf, wi, &mbs, locale);
						if (nconv == (size_t)-1) {
							goto input_failure;
						}
						if (nconv > width) {
							break;
						}
						if (!(flags & SUPPRESS)) {
							memcpy(mbp, mbbuf, nconv);
						}
					}
					if (!(flags & SUPPRESS)) {
						mbp += nconv;
					}
					width -= nconv;
					n++;
				}
				if (wi != WEOF) {
					fungetmbrune(wi, fp);
				}
				if (!(flags & SUPPRESS)) {
					*mbp = 0;
					nassigned++;
				}
			}
			nread += n;
			nconversions++;
			break;

		case CT_STRING:
			/* like CCL, but zero-length string OK, & no NOSKIP */
			if (width == 0) {
				width = (size_t)~0;
			}
			if ((flags & SUPPRESS) && (flags & LONG)) {
				wi = fgetmbrune(fp);
				while (wi != WEOF && width-- != 0 && !iswspace_l(wi, locale)) {
					nread++;
				}
				if (wi != WEOF) {
					fungetmbrune(wi, fp);
				}
			} else if (flags & LONG) {
				p0 = p = va_arg(ap, wchar_t *);
				wi = fgetmbrune(fp);
				while (wi != WEOF && width-- != 0 && !iswspace_l(wi, locale)) {
					*p++ = (wchar_t) wi;
					nread++;
				}
				if (wi != WEOF) {
					fungetmbrune(wi, fp);
				}
				*p = '\0';
				nassigned++;
			} else {
				if (!(flags & SUPPRESS)) {
					mbp = va_arg(ap, char *);
				}
				mbs = initial;
				wi = fgetmbrune(fp);
				while (wi != WEOF && width != 0 && !iswspace_l(wi, locale)) {
					if (width >= MB_CUR_MAX && !(flags & SUPPRESS)) {
						nconv = wcrtomb_l(mbp, wi, &mbs, locale);
						if (nconv == (size_t)-1) {
							goto input_failure;
						}
					} else {
						nconv = wcrtomb_l(mbbuf, wi, &mbs, locale);
						if (nconv == (size_t)-1) {
							goto input_failure;
						}
						if (nconv > width) {
							break;
						}
						if (!(flags & SUPPRESS)) {
							memcpy(mbp, mbbuf, nconv);
						}
					}
					if (!(flags & SUPPRESS)) {
						mbp += nconv;
					}
					width -= nconv;
					nread++;
				}
				if (wi != WEOF) {
					fungetmbrune(wi, fp);
				}
				if (!(flags & SUPPRESS)) {
					*mbp = 0;
					nassigned++;
				}
			}
			nconversions++;
			continue;

		case CT_INT:
			/* scan an integer as if by strtol/strtoul */
			if (width == 0 || width > sizeof(buf) / sizeof(*buf) - 1) {
				width = sizeof(buf) / sizeof(*buf) - 1;
			}
			flags |= SIGNOK | NDIGITS | NZDIGITS;
			for (p = buf; width; width--) {
				c = fgetmbrune(fp);
				/*
				 * Switch on the character; `goto ok'
				 * if we accept it as a part of number.
				 */
				switch (c) {

				/*
				 * The digit 0 is always legal, but is
				 * special.  For %i conversions, if no
				 * digits (zero or nonzero) have been
				 * scanned (only signs), we will have
				 * base==0.  In that case, we should set
				 * it to 8 and enable 0x prefixing.
				 * Also, if we have not scanned zero digits
				 * before this, do not turn off prefixing
				 * (someone else will turn it off if we
				 * have scanned any nonzero digits).
				 */
				case '0':
					if (base == 0) {
						base = 8;
						flags |= PFXOK;
					}
					if (flags & NZDIGITS) {
						flags &= ~(SIGNOK | NZDIGITS | NDIGITS);
					} else {
						flags &= ~(SIGNOK | PFXOK | NDIGITS);
					}
					goto ok;

					/* 1 through 7 always legal */
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					base = basefix[base];
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

					/* digits 8 and 9 ok iff decimal or hex */
				case '8':
				case '9':
					base = basefix[base];
					if (base <= 8) {
						break; /* not legal here */
					}
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

					/* letters ok iff hex */
				case 'A':
				case 'B':
				case 'C':
				case 'D':
				case 'E':
				case 'F':
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'e':
				case 'f':
					/* no need to fix base here */
					if (base <= 10) {
						break; /* not legal here */
					}
					flags &= ~(SIGNOK | PFXOK | NDIGITS);
					goto ok;

					/* sign ok only as first character */
				case '+':
				case '-':
					if (flags & SIGNOK) {
						flags &= ~SIGNOK;
						goto ok;
					}
					break;

					/* x ok iff flag still set & 2nd char */
				case 'x':
				case 'X':
					if ((flags & PFXOK) && p == buf + 1) {
						base = 16; /* if %i */
						flags &= ~PFXOK;
						goto ok;
					}
					break;
				}

				/*
				 * If we got here, c is not a legal character
				 * for a number.  Stop accumulating digits.
				 */
				if (c != WEOF) {
					fungetmbrune(c, fp);
				}
				break;
			ok:
				/*
				 * c is legal: store it and look at the next.
				 */
				*p++ = (wchar_t)c;
			}
			/*
			 * If we had only a sign, it is no good; push
			 * back the sign.  If the number ends in `x',
			 * it was [sign] '0' 'x', so push back the x
			 * and treat it as [sign] '0'.
			 */
			if (flags & NDIGITS) {
				if (p > buf) {
					fungetmbrune(*--p, fp);
				}
				goto match_failure;
			}
			c = p[-1];
			if (c == 'x' || c == 'X') {
				--p;
				fungetmbrune(c, fp);
			}
			if ((flags & SUPPRESS) == 0) {
				uintmax_t res;

				*p = 0;
				if ((flags & UNSIGNED) == 0) {
				    res = wcstoimax_l(buf, NULL, base, locale);
				} else {
				    res = wcstoumax_l(buf, NULL, base, locale);
				}
				if (flags & POINTER) {
					*va_arg(ap, void **) =
							(void *)(uintptr_t)res;
				} else if (flags & SHORTSHORT) {
					*va_arg(ap, char *) = (char)res;
				} else if (flags & SHORT) {
					*va_arg(ap, short *) = (short)res;
				} else if (flags & LONG) {
					*va_arg(ap, long *) = (long)res;
				} else if (flags & LONGLONG) {
					*va_arg(ap, quad_t *) = res;
				} else if (flags & INTMAXT) {
					*va_arg(ap, intmax_t *) = res;
				} else if (flags & PTRDIFFT) {
					*va_arg(ap, ptrdiff_t *) = (ptrdiff_t)res;
				} else if (flags & SIZET) {
					*va_arg(ap, size_t *) = (size_t)res;
				} else {
					*va_arg(ap, int *) = (int)res;
				}
				nassigned++;
			}
			nread += (int) (p - buf);
			nconversions++;
			break;

#ifdef FLOATING_POINT
		case CT_FLOAT:
			/* scan a floating point number as if by strtod */
			if (width == 0 || width > sizeof(buf) / sizeof(*buf) - 1) {
				width = sizeof(buf) / sizeof(*buf) - 1;
			}
			width = parsefloat(fp, buf, buf + width, locale);
			if (width == 0) {
				goto match_failure;
			}
			if ((flags & SUPPRESS) == 0) {
				if (flags & LONGDBL) {
					long double res = wcstold_l(buf, &p, locale);
					*va_arg(ap, long double *) = res;
				} else if (flags & LONG) {
					double res = wcstod_l(buf, &p, locale);
					*va_arg(ap, double *) = res;
				} else {
					float res = wcstof_l(buf, &p, locale);
					*va_arg(ap, float *) = res;
				}
#ifdef DEBUG
				if (p - buf != (ptrdiff_t)width) {
					abort();
				}
#endif
				nassigned++;
			}
			nread += width;
			nconversions++;
			break;
#endif /* FLOATING_POINT */
		}
	}

input_failure:
	return (nconversions != 0 ? nassigned : EOF);
match_failure:
	return (nassigned);
}

/*
 * Fill in the given table from the scanset at the given format
 * (just after `[').  Return a pointer to the character past the
 * closing `]'.  The table has a 1 wherever characters should be
 * considered part of the scanset.
 */
static const wchar_t *
swcl(wchar_t *tab, const wchar_t *fmt, int *compl, locale_t locale)
{
	return (usccl(tab, fmt, compl, locale));
}

#ifdef FLOATING_POINT
static int
parsefloat(FILE *fp, wchar_t *buf, wchar_t *end, locale_t locale)
{
	wchar_t *commit, *p;
	int infnanpos = 0;
	enum {
		S_START, S_GOTSIGN, S_INF, S_NAN, S_MAYBEHEX,
		S_DIGITS, S_FRAC, S_EXP, S_EXPDIGITS
	} state = S_START;
	wchar_t c;
	wchar_t decpt = (wchar_t)(unsigned char)*localeconv_l(locale)->decimal_point;
	int gotmantdig = 0, ishex = 0;

	/*
	 * We set commit = p whenever the string we have read so far
	 * constitutes a valid representation of a floating point
	 * number by itself.  At some point, the parse will complete
	 * or fail, and we will ungetc() back to the last commit point.
	 * To ensure that the file offset gets updated properly, it is
	 * always necessary to read at least one character that doesn't
	 * match; thus, we can't short-circuit "infinity" or "nan(...)".
	 */
	commit = buf - 1;
	c = WEOF;
	for (p = buf; p < end; ) {
		c = fgetmbrune(fp);
		if (c == WEOF) {
			break;
		}
reswitch:
		switch (state) {
		case S_START:
			state = S_GOTSIGN;
			if (c == '-' || c == '+') {
				break;
			} else {
				goto reswitch;
			}
		case S_GOTSIGN:
			switch (c) {
			case '0':
				state = S_MAYBEHEX;
				commit = p;
				break;
			case 'I':
			case 'i':
				state = S_INF;
				break;
			case 'N':
			case 'n':
				state = S_NAN;
				break;
			default:
				state = S_DIGITS;
				goto reswitch;
			}
			break;
		case S_INF:
			if (infnanpos > 6
					|| (c != "nfinity"[infnanpos] && c != "NFINITY"[infnanpos])) {
				goto parsedone;
			}
			if (infnanpos == 1 || infnanpos == 6) {
				commit = p; /* inf or infinity */
			}
			infnanpos++;
			break;
		case S_NAN:
			switch (infnanpos) {
			case -1: /* XXX kludge to deal with nan(...) */
				goto parsedone;
			case 0:
				if (c != 'A' && c != 'a') {
					goto parsedone;
				}
				break;
			case 1:
				if (c != 'N' && c != 'n') {
					goto parsedone;
				} else {
					commit = p;
				}
				break;
			case 2:
				if (c != '(') {
					goto parsedone;
				}
				break;
			default:
				if (c == ')') {
					commit = p;
					infnanpos = -2;
				} else if (!iswalnum_l(c, locale) && c != '_') {
					goto parsedone;
				}
				break;
			}
			infnanpos++;
			break;
		case S_MAYBEHEX:
			state = S_DIGITS;
			if (c == 'X' || c == 'x') {
				ishex = 1;
				break;
			} else { /* we saw a '0', but no 'x' */
				gotmantdig = 1;
				goto reswitch;
			}
		case S_DIGITS:
			if ((ishex && iswxdigit_l(c, locale)) || iswdigit_l(c, locale)) {
				gotmantdig = 1;
			} else {
				state = S_FRAC;
				if (c != decpt) {
					goto reswitch;
				}
			}
			if (gotmantdig) {
				commit = p;
			}
			break;
		case S_FRAC:
			if (((c == 'E' || c == 'e') && !ishex)
					|| ((c == 'P' || c == 'p') && ishex)) {
				if (!gotmantdig) {
					goto parsedone;
				} else {
					state = S_EXP;
				}
			} else if ((ishex && iswxdigit_l(c, locale))
					|| iswdigit_l(c, locale)) {
				commit = p;
				gotmantdig = 1;
			} else {
				goto parsedone;
			}
			break;
		case S_EXP:
			state = S_EXPDIGITS;
			if (c == '-' || c == '+') {
				break;
			} else {
				goto reswitch;
			}
		case S_EXPDIGITS:
			if (iswdigit_l(c, locale)) {
				commit = p;
			} else {
				goto parsedone;
			}
			break;
		default:
			abort();
		}
		*p++ = c;
		c = WEOF;
	}

parsedone:
	if (c != WEOF) {
		fungetmbrune(c, fp);
	}
	while (commit < --p) {
		fungetmbrune(*p, fp);
	}
	*++commit = '\0';
	return ((int)(commit - buf));
}

#endif /* FLOATING_POINT */
