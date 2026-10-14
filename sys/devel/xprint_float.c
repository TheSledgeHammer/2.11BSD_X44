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
#include <sys/types.h>

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#include "xprint.h"

#ifdef FLOATING_POINT
#include <locale.h>
#include <math.h>
#include "floatio.h"

static char *ldcvt(long double, const char *, int, int, int, CHAR_T *, int *, int, int *);
static char *dcvt(double, const char *, int, int, int, CHAR_T *, int *, int, int *);

/*
 * Initialize the thousands' grouping state in preparation to print a
 * number with ndigits digits. This routine returns the total number
 * of wide characters that will be printed.
 */
int
grouping_init(int ndigits, int nseps, int nrepeats, const char *grouping)
{
	int lead; /* sig figs before decimal or group sep */

	lead = ndigits;
	while (*grouping != CHAR_MAX) {
		if (lead <= *grouping) {
			break;
		}
		lead -= *grouping;
		if (*(grouping + 1)) {
			nseps++;
			grouping++;
		} else {
			nrepeats++;
		}
	}
	return (nseps + nrepeats);
}

static char *
ldcvt(long double value, const char *xdigs, int mode, int ndigits, int flags, CHAR_T *sign, int *decpt, int ch, int *length)
{
	int dsgn;
	char *digits, *bp, *rve;

	digits = NULL;
	dsgn = 0;
	if ((ch == 'a' || ch == 'A')) {
		digits = __hldtoa(value, xdigs, ndigits, decpt, &dsgn, &rve);
		goto out;
	}
	if ((ch == 'g' || ch == 'G')) {
		digits = __ldtoa(&value, mode, ndigits, decpt, &dsgn, &rve);
		goto out;
	}

out:
	if (dsgn) {
		value = -value;
		*sign = '-';
	} else {
		*sign = '\000';
	}
	if (((ch != 'a' && ch != 'A') && (ch != 'g' && ch != 'G'))
			|| (flags & ALT)) { /* Print trailing zeros */
		bp = digits + ndigits;
		if (ch == 'f') {
			if (*digits == '0' && value) {
				*decpt = -ndigits + 1;
			}
			bp += *decpt;
		}
		if (value == 0) { /* kludge for __dtoa irregularity */
			rve = bp;
		}
		while (rve < bp) {
			*rve++ = '0';
		}
	}
	*length = rve - digits;
	return (digits);
}

static char *
dcvt(double value, const char *xdigs, int mode, int ndigits, int flags, CHAR_T *sign, int *decpt, int ch, int *length)
{
	int dsgn;
	char *digits, *bp, *rve;

    digits = NULL;
	dsgn = 0;
	if ((ch == 'a' || ch == 'A')) {
		digits = __hdtoa(value, xdigs, ndigits, decpt, &dsgn, &rve);
		goto out;
	}
	if ((ch == 'g' || ch == 'G')) {
		digits = __dtoa(value, mode, ndigits, decpt, &dsgn, &rve);
		if (*decpt == 9999) {
			*decpt = INT_MAX;
		}
		goto out;
	}

out:
	if (dsgn) {
		value = -value;
		*sign = '-';
	} else {
		*sign = '\000';
	}
	if (((ch != 'a' && ch != 'A') && (ch != 'g' && ch != 'G'))
			|| (flags & ALT)) { /* Print trailing zeros */
		bp = digits + ndigits;
		if (ch == 'f') {
			if (*digits == '0' && value) {
				*decpt = -ndigits + 1;
			}
			bp += *decpt;
		}
		if (value == 0) {/* kludge for __dtoa irregularity */
			rve = bp;
		}
		while (rve < bp) {
			*rve++ = '0';
		}
	}
	*length = rve - digits;
	return (digits);
}

char *
cvt(union fparg *fp, const char *xdigs, int ndigits, int flags, CHAR_T *sign, int *decpt, int ch, int *length)
{
	int mode;
	char *digits;

	_DIAGASSERT(decpt != NULL);
	_DIAGASSERT(length != NULL);
	_DIAGASSERT(sign != NULL);

	if (ch == 'f' || ch == 'F') {
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

	if ((flags & LONGDBL)) {
		digits = ldcvt(fp->ldbl, xdigs, mode, ndigits, flags, sign, decpt, ch,
				length);
	} else {
		digits = dcvt(fp->dbl, xdigs, mode, ndigits, flags, sign, decpt, ch,
				length);
	}
	return (digits);
}

int
exponent(CHAR_T *p0, int exp, int fmtch)
{
	CHAR_T *p, *t;
	CHAR_T expbuf[MAXEXP];

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
