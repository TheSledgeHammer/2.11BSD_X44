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
__FBSDID("$FreeBSD: src/lib/libc/stdio/vfscanf.c,v 1.41 2007/01/09 00:28:07 imp Exp $");
__RCSID("$NetBSD: vfscanf.c,v 1.47 2022/05/24 06:27:59 andvar Exp $");
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

#define NARROW
#include "xscan.h"

#ifdef FLOATING_POINT
#include "floatio.h"
#endif

int __scanfdebug = 0;

#ifdef FLOATING_POINT
static size_t parsefloat(FILE *, char *, char *, locale_t);
#endif
static const char *sccl(char *, const char *, int *, locale_t);

int vfscanf_l(FILE *, locale_t, const char *, va_list);
int __svfscanf_l(FILE *, locale_t, const char *, va_list);
int __svfscanf_l_unlocked(FILE *, locale_t, const char *, va_list);

int
vfscanf(FILE *fp, const char *fmt0, va_list ap)
{
	return (__svfscanf(fp, fmt0, ap));
}

int
vfscanf_l(FILE *fp, locale_t locale, const char *fmt0, va_list ap)
{
	return (__svfscanf_l(fp, locale, fmt0, ap));
}

int
__svfscanf(FILE *fp, const char *fmt0, va_list ap)
{
	return (__svfscanf_l(fp, __get_locale(), fmt0, ap));
}

int
__svfscanf_l(FILE *fp, locale_t locale, const char *fmt0, va_list ap)
{
	int ret;

	FLOCKFILE(fp);
	ret = __svfscanf_unlocked_l(fp, locale, fmt0, ap);
	FUNLOCKFILE(fp);
	return (ret);
}

#define SCANF_SKIP_SPACE() \
do { \
	while ((fp->_r > 0 || __srefill(fp) == 0) && isspace_l(*fp->_p, locale)) \
		nread++, fp->_r--, fp->_p++; \
} while (0)


int
__svfscanf_l_unlocked(FILE *fp, locale_t locale, const char *fmt0, va_list ap)
{
	const char *fmt;
	int c;					/* character from format, or conversion */
	size_t width;			/* field width, or 0 */
	char *p;				/* points into all kinds of strings */
	size_t n;				/* handy integer */
	int flags;				/* flags as defined above */
	char *p0;				/* saves original value of p when necessary */
	int nassigned;			/* number of fields assigned */
	int nconversions;		/* number of conversions */
	size_t nread;			/* number of characters consumed from fp */
	int base;				/* base argument to strtol/strtoul */
	char ccltab[CCLTABMAXLEN]; /* character class table for %[...] */
	char buf[BUF];			/* buffer for numeric conversions */
	int cclcompl;			/* ccl is complemented? */
	wchar_t *wcp;			/* handy wide-character pointer */
	size_t nconv;			/* length of multibyte sequence converted */
	static const mbstate_t initial;
	mbstate_t mbs;

	/* `basefix' is used to avoid `if' tests in the integer scanner */
	static short basefix[17] =
		{ 10, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };

	_DIAGASSERT(fp != NULL);
	_DIAGASSERT(fmt0 != NULL);

    _SET_ORIENTATION(fp, -1);

	fmt = fmt0;
	nassigned = 0;
	nconversions = 0;
	nread = 0;
	base = 0;		/* XXX just to keep gcc happy */
	for (;;) {
		c = *fmt++;
		if (c == 0) {
			return (nassigned);
		}
		if (isspace_l(c, locale)) {
			for (;;) {
				if (fp->_r <= 0 && __srefill(fp)) {
					return (nassigned);
				}
				if (!isspace_l(*fp->_p, locale)) {
					break;
				}
				nread++, fp->_r--, fp->_p++;
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
			if (fp->_r <= 0 && __srefill(fp)) {
				goto input_failure;
			}
			if (*fp->_p != c) {
				goto match_failure;
			}
			fp->_r--, fp->_p++;
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
			flags |= LONGLONG;	/* not quite */
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
			flags |= UNSIGNED;
			base = 8;
			break;

		case 'u':
			c = CT_INT;
			flags |= UNSIGNED;
			base = 10;
			break;

		case 'X': /* compat   XXX */
			flags |= LONG;
			/* FALLTHROUGH */
		case 'x':
			flags |= PFXOK; /* enable 0x prefixing */
			c = CT_INT;
			flags |= UNSIGNED;
			base = 16;
			break;

#ifdef FLOATING_POINT
		case 'A':
		case 'E':	/* compat   XXX */
		case 'F':	/* compat */
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
			fmt = sccl(ccltab, fmt, &cclcompl);
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
			flags |= UNSIGNED;	/*      >= sizeof(uintptr_t) */
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
		 * We have a conversion that requires input.
		 */
		if (fp->_r <= 0 && __srefill(fp))
			goto input_failure;

		/*
		 * Consume leading white space, except for formats
		 * that suppress this.
		 */
		if ((flags & NOSKIP) == 0) {
			while (isspace_l(*fp->_p, locale)) {
				nread++;
				if (--fp->_r > 0) {
					fp->_p++;
				} else if (__srefill(fp)) {
					goto input_failure;
				}
			}
			/*
			 * Note that there is at least one character in
			 * the buffer, so conversions that do not set NOSKIP
			 * ca no longer result in an input failure.
			 */
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
				if ((flags & SUPPRESS) == 0) {
					wcp = va_arg(ap, wchar_t *);
				} else {
					wcp = NULL;
				}
				n = 0;
				while (width != 0) {
					if (n == MB_CUR_MAX) {
						fp->_flags |= __SERR;
						goto input_failure;
					}
					buf[n++] = *fp->_p;
					fp->_p++;
					fp->_r--;
					mbs = initial;
					nconv = mbrtowc_l(wcp, buf, n, &mbs, locale);
					if (nconv == (size_t)-1) {
						fp->_flags |= __SERR;
						goto input_failure;
					}
					if (nconv == 0 && !(flags & SUPPRESS)) {
						*wcp = L'\0';
					}
					if (nconv != (size_t)-2) {
						nread += n;
						width--;
						if (!(flags & SUPPRESS)) {
							wcp++;
						}
						n = 0;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						if (n != 0) {
							fp->_flags |= __SERR;
							goto input_failure;
						}
						break;
					}
				}
				if (!(flags & SUPPRESS)) {
					nassigned++;
				}
			} else if (flags & SUPPRESS) {
				size_t sum = 0;
				for (;;) {
					if ((n = fp->_r) < width) {
						sum += n;
						width -= n;
						fp->_p += n;
						if (__srefill(fp)) {
							if (sum == 0) {
								goto input_failure;
							}
							break;
						}
					} else {
						sum += width;
						fp->_r -= (int) width;
						fp->_p += width;
						break;
					}
				}
				nread += sum;
			} else {
				size_t r = fread(va_arg(ap, char *), 1, width, fp);
				if (r == 0) {
					goto input_failure;
				}
				nread += r;
				nassigned++;
			}
			nconversions++;
			break;

		case CT_CCL:
			/* scan a (nonempty) character class (sets NOSKIP) */
			if (width == 0) {
				width = (size_t)~0; /* `infinity' */
			}
			/* take only those things in the class */
			if (flags & LONG) {
				wchar_t twc;
				int nchars;

				if ((flags & SUPPRESS) == 0) {
					wcp = va_arg(ap, wchar_t *);
				} else {
					wcp = &twc;
				}
				n = 0;
				nchars = 0;
				while (width != 0) {
					if (n == MB_CUR_MAX) {
						fp->_flags |= __SERR;
						goto input_failure;
					}
					buf[n++] = *fp->_p;
					fp->_p++;
					fp->_r--;
					mbs = initial;
					nconv = mbrtowc_l(wcp, buf, n, &mbs, locale);
					if (nconv == (size_t)-1) {
						fp->_flags |= __SERR;
						goto input_failure;
					}
					if (nconv == 0) {
						*wcp = L'\0';
					}
					if (nconv != (size_t)-2) {
						if (wctob_l(*wcp, locale) != EOF
								&& !ccltab[wctob_l(*wcp, locale)]) {
							while (n != 0) {
								n--;
								(void)ungetc(buf[n], fp);
							}
							break;
						}
						nread += n;
						width--;
						if (!(flags & SUPPRESS)) {
							wcp++;
						}
						nchars++;
						n = 0;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						if (n != 0) {
							fp->_flags |= __SERR;
							goto input_failure;
						}
						break;
					}
				}
				if (n != 0) {
					fp->_flags |= __SERR;
					goto input_failure;
				}
				n = nchars;
				if (n == 0)
					goto match_failure;
				if (!(flags & SUPPRESS)) {
					*wcp = L'\0';
					nassigned++;
				}
			} else if (flags & SUPPRESS) {
				n = 0;
				while (ccltab[*fp->_p]) {
					n++, fp->_r--, fp->_p++;
					if (--width == 0) {
						break;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						if (n == 0) {
							goto input_failure;
						}
						break;
					}
				}
				if (n == 0) {
					goto match_failure;
				}
			} else {
				p0 = p = va_arg(ap, char *);
				while (ccltab[*fp->_p]) {
					fp->_r--;
					*p++ = *fp->_p++;
					if (--width == 0) {
						break;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						if (p == p0) {
							goto input_failure;
						}
						break;
					}
				}
				n = p - p0;
				if (n == 0) {
					goto match_failure;
				}
				*p = 0;
				nassigned++;
			}
			nread += n;
			nconversions++;
			break;

		case CT_STRING:
			/* like CCL, but zero-length string OK, & no NOSKIP */
			if (width == 0) {
				width = (size_t)~0;
			}
			if (flags & LONG) {
				wchar_t twc;

				if ((flags & SUPPRESS) == 0) {
					wcp = va_arg(ap, wchar_t *);
				} else {
					wcp = &twc;
				}
				n = 0;
				while (!isspace_l(*fp->_p, locale) && width != 0) {
					if (n == MB_CUR_MAX) {
						fp->_flags |= __SERR;
						goto input_failure;
					}
					buf[n++] = *fp->_p;
					fp->_p++;
					fp->_r--;
					mbs = initial;
					nconv = mbrtowc_l(wcp, buf, n, &mbs, locale);
					if (nconv == (size_t)-1) {
						fp->_flags |= __SERR;
						goto input_failure;
					}
					if (nconv == 0) {
						*wcp = L'\0';
					}
					if (nconv != (size_t)-2) {
						if (iswspace_l(*wcp, locale)) {
							while (n != 0) {
								n--;
								(void)ungetc(buf[n], fp);
							}
							break;
						}
						nread += n;
						width--;
						if (!(flags & SUPPRESS)) {
							wcp++;
						}
						n = 0;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						if (n != 0) {
							fp->_flags |= __SERR;
							goto input_failure;
						}
						break;
					}
				}
				if (!(flags & SUPPRESS)) {
					*wcp = L'\0';
					nassigned++;
				}
			} else if (flags & SUPPRESS) {
				n = 0;
				while (!isspace_l(*fp->_p, locale)) {
					n++, fp->_r--, fp->_p++;
					if (--width == 0) {
						break;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						break;
					}
				}
				nread += n;
			} else {
				p0 = p = va_arg(ap, char*);
				while (!isspace_l(*fp->_p, locale)) {
					fp->_r--;
					*p++ = *fp->_p++;
					if (--width == 0) {
						break;
					}
					if (fp->_r <= 0 && __srefill(fp)) {
						break;
					}
				}
				*p = 0;
				nread += p - p0;
				nassigned++;
			}
			nconversions++;
			continue;

		case CT_INT:
			/* scan an integer as if by strtol/strtoul */
#ifdef hardway
			if (width == 0 || width > sizeof(buf) - 1) {
				width = sizeof(buf) - 1;
			}
#else
			/* size_t is unsigned, hence this optimisation */
			if (--width > sizeof(buf) - 2) {
				width = sizeof(buf) - 2;
			}
			width++;
#endif
			flags |= SIGNOK | NDIGITS | NZDIGITS;
			for (p = buf; width; width--) {
				c = *fp->_p;
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
				break;
		ok:
				/*
				 * c is legal: store it and look at the next.
				 */
				*p++ = c;
				if (--fp->_r > 0) {
					fp->_p++;
				} else if (__srefill(fp)) {
					break; /* EOF */
				}
			}
			/*
			 * If we had only a sign, it is no good; push
			 * back the sign.  If the number ends in `x',
			 * it was [sign] '0' 'x', so push back the x
			 * and treat it as [sign] '0'.
			 */
			if (flags & NDIGITS) {
				if (p > buf) {
					(void)ungetc(*(u_char *)--p, fp);
				}
				goto match_failure;
			}
			c = ((u_char *)p)[-1];
			if (c == 'x' || c == 'X') {
				--p;
				(void)ungetc(c, fp);
			}
			if ((flags & SUPPRESS) == 0) {
				uintmax_t res;

				*p = 0;
				if ((flags & UNSIGNED) == 0) {
					res = strtoimax_l(buf, (char **) NULL, base, locale);
				} else {
					res = strtoumax_l(buf, (char **) NULL, base, locale);
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
					*va_arg(ap, long long *) = res;
				} else if (flags & INTMAXT) {
					*va_arg(ap, intmax_t *) = res;
				} else if (flags & PTRDIFFT) {
					*va_arg(ap, ptrdiff_t *) =
					    (ptrdiff_t)res;
				} else if (flags & SIZET) {
					*va_arg(ap, size_t *) = (size_t)res;
				} else {
					*va_arg(ap, int *) = (int)res;
				}
				nassigned++;
			}
			nread += p - buf;
			nconversions++;
			break;

#ifdef FLOATING_POINT
		case CT_FLOAT:
			/* scan a floating point number as if by strtod */
			if (width == 0 || width > sizeof(buf) - 1) {
				width = sizeof(buf) - 1;
			}
			width = parsefloat(fp, buf, buf + width, locale);
			if (width == 0) {
				goto match_failure;
			}
			if ((flags & SUPPRESS) == 0) {
				if (flags & LONGDBL) {
					long double res = strtold_l(buf, &p, locale);
					*va_arg(ap, long double *) = res;
				} else if (flags & LONG) {
					double res = strtod_l(buf, &p, locale);
					*va_arg(ap, double *) = res;
				} else {
					float res = strtof_l(buf, &p, locale);
					*va_arg(ap, float *) = res;
				}
				if (__scanfdebug && (size_t)(p - buf) != width) {
					abort();
				}
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
static const char *
sccl(char *tab, const char *fmt, int *compl, locale_t locale)
{
	return ((const char *)usccl((u_char *)tab, (const u_char *)fmt, compl, locale));
}

#ifdef FLOATING_POINT

static size_t
parsefloat(FILE *fp, char *buf, char *end, locale_t locale)
{
	char *commit, *p;
	int infnanpos = 0;
	enum {
		S_START, S_GOTSIGN, S_INF, S_NAN, S_MAYBEHEX,
		S_DIGITS, S_FRAC, S_EXP, S_EXPDIGITS
	} state = S_START;
	unsigned char c;
	char decpt = *localeconv_l(locale)->decimal_point;
	_Bool gotmantdig = 0, ishex = 0;

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
	for (p = buf; p < end; ) {
		c = *fp->_p;
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
				} else if (!isalnum_l(c, locale) && c != '_') {
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
			if ((ishex && isxdigit_l(c, locale)) || isdigit_l(c, locale)) {
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
			} else if ((ishex && isxdigit_l(c, locale)) || isdigit_l(c, locale)) {
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
			if (isdigit_l(c, locale)) {
				commit = p;
			} else {
				goto parsedone;
			}
			break;
		default:
			abort();
		}
		*p++ = c;
		if (--fp->_r > 0) {
			fp->_p++;
		} else if (__srefill(fp)) {
			break; /* EOF */
		}
	}

parsedone:
	while (commit < --p) {
		(void)ungetc(*(u_char *)p, fp);
	}
	*++commit = '\0';
	return (commit - buf);
}

#endif /* FLOATING_POINT */
